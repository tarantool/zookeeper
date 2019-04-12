#include "driver.h"

#include <string.h>

#ifndef ZOO_NOTCONNECTED_STATE
#  define ZOO_NOTCONNECTED_STATE 999
#endif

#ifndef ZOO_READONLY_STATE
#  define ZOO_READONLY_STATE 5
#endif

static int
_zk_build_stat(lua_State *L, const struct Stat *stat);

static int
_zk_build_string_vector(lua_State *L, const struct String_vector *sv);

static int
_zk_copy_acl_list(lua_State *L, const struct ACL_vector *acls);

static void
_zk_local_wctx_free(lua_State *L, struct zk_local_wctx *wctx);


static inline struct lua_zoo_handle *
_zk_check_zoo_handle(struct lua_State *L, int index)
{
    struct lua_zoo_handle *handle = luaL_checkudata(L, index, ZOOKEEP_MT_NAME);
    if (handle->zh != NULL) {
        return handle;
    }
    
    luaL_error(L, "invalid zookeeper handle.");
    return NULL;
}

static inline struct lua_zoo_handle *
_zk_check_zoo_handle_connected(struct lua_State *L, int index)
{
    struct lua_zoo_handle *handle =_zk_check_zoo_handle(L, index);
    if (handle->zh == NULL) {
        return NULL;
    }
    
    if (zoo_state(handle->zh) == ZOO_CONNECTED_STATE) {
        return handle;
    }
    luaL_error(L, "zookeeper not connected");
    return NULL;
}

static inline struct ACL_vector *
_zk_check_zoo_acl(struct lua_State *L, int index)
{
    struct ACL_vector *a = luaL_checkudata(L, index, ZOOKEEP_ACL_LIST_MT_NAME);
    if (a != NULL) {
        return a;
    }
    
    luaL_error(L, "invalid ACL vector object.");
    return NULL;
}

/***************** cb functions *****************/

void
watcher_dispatch(zhandle_t *zh,
                 int type,
                 int state,
                 const char *path,
                 void *watcherctx)
{
    (void) zh;
    struct zk_global_wctx *wctx = (struct zk_global_wctx *) watcherctx;
    lua_State *L = wctx->L;
    int cbref = wctx->cbref;
    int internal_ctx_ref = wctx->internal_ctx_ref;
    int user_ctx_ref = wctx->user_ctx_ref;

    say_debug("Global watcher dispatch. L=%p cbref=%d internal_ctx_ref=%d user_ctx_ref=%d | type=%d state=%d path=%s", 
             L, cbref, internal_ctx_ref, user_ctx_ref, type, state, path);

    /** push lua watcher_fn onto the stack. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbref);
    /** push internal ctx onto the stack (it should be a zokeep object). */
    lua_rawgeti(L, LUA_REGISTRYINDEX, internal_ctx_ref);
    /** push type onto the stack. */
    lua_pushinteger(L, type);
    /** push state onto the stack. */
    lua_pushinteger(L, state);
    /** push path onto the stack. */
    lua_pushstring(L, path);
    /** push user ctx onto the stack. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, user_ctx_ref);
    lua_call(L, 5, 0);
}

void
local_watcher_dispatch(zhandle_t *zh,
                       int type,
                       int state,
                       const char *path,
                       void *watcherctx)
{
    (void) zh;
    struct zk_local_wctx *wctx = (struct zk_local_wctx *) watcherctx;
    lua_State *L = wctx->L;
    int cbref = wctx->cbref;
    int internal_ctx_ref = wctx->internal_ctx_ref;
    int user_ctx_ref = wctx->user_ctx_ref;

    /** push lua watcher_fn onto the stack. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbref);
    /* push internal ctx onto the stack (it should be a zookeep object). */
    lua_rawgeti(L, LUA_REGISTRYINDEX, internal_ctx_ref);
    /** push type onto the stack. */
    lua_pushinteger(L, type);
    /** push state onto the stack. */
    lua_pushinteger(L, state);
    /** push path onto the stack. */
    lua_pushstring(L, path);
    /** push watcher context onto the stack. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, user_ctx_ref);
    lua_call(L, 5, 0);
    
    _zk_local_wctx_free(L, wctx);
}

void
_zk_void_cb(int rc,
            const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    lua_pushinteger(L, rc);
    cdata->ret_count = 1;
    fiber_cond_signal(cdata->cond);
}

void
_zk_data_cb(int rc,
            const char *value,
            int value_len,
            const struct Stat *stat,
            const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    if (value == NULL) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, value, value_len);
    }
    _zk_build_stat(L, stat);
    lua_pushinteger(L, rc);
    
    cdata->ret_count = 3;
    fiber_cond_signal(cdata->cond);
}

void
_zk_stat_cb(int rc,
            const struct Stat *stat,
            const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    lua_pushboolean(L, stat != NULL);
    _zk_build_stat(L, stat);
    lua_pushinteger(L, rc);
    
    cdata->ret_count = 3;
    fiber_cond_signal(cdata->cond);
}

void
_zk_string_cb(int rc,
              const char *value,
              const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    lua_pushstring(L, value);
    lua_pushinteger(L, rc);
    
    cdata->ret_count = 2;
    fiber_cond_signal(cdata->cond);
}

void
_zk_strings_cb(int rc,
               const struct String_vector *strings,
               const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    _zk_build_string_vector(L, strings);
    lua_pushinteger(L, rc);
    
    cdata->ret_count = 2;
    fiber_cond_signal(cdata->cond);
}

void
_zk_strings_stat_cb(int rc,
                    const struct String_vector *strings,
                    const struct Stat *stat,
                    const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    _zk_build_string_vector(L, strings);
    _zk_build_stat(L, stat);
    lua_pushinteger(L, rc);
    
    cdata->ret_count = 3;
    fiber_cond_signal(cdata->cond);
}

void
_zk_acl_cb(int rc,
           struct ACL_vector *acl,
           struct Stat *stat,
           const void *data)
{
    struct zk_data_result *cdata = (struct zk_data_result *) data;
    lua_State *L = cdata->L;
    
    _zk_copy_acl_list(L, acl);
    _zk_build_stat(L, stat);
    lua_pushinteger(L, rc);
    
    cdata->ret_count = 3;
    fiber_cond_signal(cdata->cond);
}

static int
_zk_build_stat(lua_State *L,
               const struct Stat *stat)
{
    lua_newtable(L);
    lua_pushstring(L, "czxid");
    lua_pushnumber(L, stat ? stat->czxid : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "mzxid");
    lua_pushnumber(L, stat ? stat->mzxid : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "ctime");
    lua_pushnumber(L, stat ? stat->ctime : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "mtime");
    lua_pushnumber(L, stat ? stat->mtime : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "version");
    lua_pushnumber(L, stat ? stat->version : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "cversion");
    lua_pushnumber(L, stat ? stat->cversion : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "aversion");
    lua_pushnumber(L, stat ? stat->aversion : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "ephemeralOwner");
    lua_pushnumber(L, stat ? stat->ephemeralOwner : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "dataLength");
    lua_pushnumber(L, stat ? stat->dataLength : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "numChildren");
    lua_pushnumber(L, stat ? stat->numChildren : 0);
    lua_settable(L, -3);
    lua_pushstring(L, "pzxid");
    lua_pushnumber(L, stat ? stat->pzxid : 0);
    lua_settable(L, -3);
    return 1;
}

static int
_zk_build_string_vector(lua_State *L,
                        const struct String_vector *sv)
{
    int i;
    if (sv != NULL) {
        lua_newtable(L);
        for (i = 0; i < sv->count; ++i) {
            lua_pushstring(L, sv->data[i]);
            lua_rawseti(L, -2, i + 1);
        }
    } else {
        lua_pushnil(L);
    }
    return 0;
}


/***************** ACL begin *****************/

static int
_zk_copy_acl_list(lua_State *L,
                  const struct ACL_vector *acls)
{
    if (acls == NULL) {
        lua_pushnil(L);
        return 1;
    }
    
    struct ACL_vector *zoo_acl =
        (struct ACL_vector *) lua_newuserdata(L, sizeof(struct ACL_vector));
    luaL_getmetatable(L, ZOOKEEP_ACL_LIST_MT_NAME);
    lua_setmetatable(L, -2);
    
    zoo_acl->count = acls->count;
    zoo_acl->data = (struct ACL *) calloc(acls->count, sizeof(struct ACL));
    if (zoo_acl->data == NULL) {
        return luaL_error(L, "zookeep: out of memory");
    }
    
    int i;
    for (i = 0; i < zoo_acl->count; ++i) {
        zoo_acl->data[i].perms = acls->data[i].perms;
        zoo_acl->data[i].id.scheme = strdup(acls->data[i].id.scheme);
        zoo_acl->data[i].id.id = strdup(acls->data[i].id.id);
    }
    return 1;
}

static int
lua_zoo_build_acl_list(lua_State *L)
{
    struct ACL_vector *zoo_acl =
        (struct ACL_vector *) lua_newuserdata(L, sizeof(struct ACL_vector));
    
    luaL_getmetatable(L, ZOOKEEP_ACL_LIST_MT_NAME);
    lua_setmetatable(L, -2);
    
    int count = 0,
        i = 0,
        L_index = 1;
        
    luaL_checktype(L, L_index, LUA_TTABLE);
    count = lua_objlen(L, L_index);
    if (count == 0) {
        zoo_acl->count = 0;
        zoo_acl->data = NULL;
        return 1;
    }
    zoo_acl->count = count;
    zoo_acl->data = (struct ACL *) calloc(count, sizeof(struct ACL));
    if (zoo_acl->data == NULL) {
        return luaL_error(L, "zookeep: out of memory");
    }
    for (i = 1; i <= count; i++) {
        lua_rawgeti(L, L_index, i);

        lua_pushstring(L, "perms");
        lua_rawget(L, -2);
        zoo_acl->data[i-1].perms = luaL_checkint(L, -1);
        lua_pop(L, 1);

        lua_pushstring(L, "scheme");
        lua_rawget(L, -2);
        zoo_acl->data[i-1].id.scheme = strdup(luaL_checkstring(L, -1));
        lua_pop(L, 1);

        lua_pushstring(L, "id");
        lua_rawget(L, -2);
        zoo_acl->data[i-1].id.id = strdup(luaL_checkstring(L, -1));
        lua_pop(L, 1);

        lua_pop(L, 1);
    }
    
    return 1;
}

static int
lua_zoo_destroy_acl_list(lua_State *L)
{
    struct ACL_vector *zoo_acl =
        luaL_checkudata(L, 1, ZOOKEEP_ACL_LIST_MT_NAME);
        
    if (zoo_acl == NULL) {
        return 0;
    }
    
    int i;
    for (i = 0; i < zoo_acl->count; ++i) {
        free(zoo_acl->data[i].id.id);
        free(zoo_acl->data[i].id.scheme);
    }
    free(zoo_acl->data);
    return 0;
}

static int
lua_zoo_acl_tostring(lua_State *L)
{
    struct ACL_vector *zoo_acl = _zk_check_zoo_acl(L, 1);
    
    static char buf[1024] = { 0 };
    int i;
    int offset = 0;
    for (i = 0; i < zoo_acl->count; ++i) {
        offset += sprintf(&buf[offset],
                          "{perms=%d, scheme=%s, id=%s}%s",
                          zoo_acl->data[i].perms,
                          zoo_acl->data[i].id.scheme,
                          zoo_acl->data[i].id.id,
                          i < zoo_acl->count-1 ? ", " : "");
    }
    lua_pushfstring(L, "ACLList [%s]", buf);
    return 1;
}

static int
lua_zoo_acl_totable(lua_State *L)
{
    struct ACL_vector *zoo_acl = _zk_check_zoo_acl(L, 1);
    
    if (zoo_acl != NULL) {
        int i;
        lua_newtable(L);
        for (i = 0; i < zoo_acl->count; ++i) {
            lua_newtable(L);
            lua_pushstring(L, "perms");
            lua_pushnumber(L, zoo_acl->data[i].perms);
            lua_settable(L, -3);
            lua_pushstring(L, "scheme");
            lua_pushstring(L, zoo_acl->data[i].id.scheme);
            lua_settable(L, -3);
            lua_pushstring(L, "id");
            lua_pushstring(L, zoo_acl->data[i].id.id);
            lua_settable(L, -3);
            lua_rawseti(L, -2, i + 1);
        }
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
lua_zoo_acl_eq(lua_State *L)
{
    struct ACL_vector *acl1 = _zk_check_zoo_acl(L, 1);
    struct ACL_vector *acl2 = _zk_check_zoo_acl(L, 2);
    
    if (acl1 == NULL && acl2 == NULL) {
        lua_pushboolean(L, 1);
        return 1;
    }
    
    if (acl1 == NULL || acl2 == NULL) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    if (acl1->count != acl2-> count) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    int i;
    for (i = 0; i < acl1->count; ++i) {
        if (acl1->data[i].perms != acl2->data[i].perms) {
            lua_pushboolean(L, 0);
            return 1;
        }
        
        int scheme1_len = strlen(acl1->data[i].id.scheme);
        int scheme2_len = strlen(acl2->data[i].id.scheme);
        
        if (scheme1_len != scheme2_len) {
            lua_pushboolean(L, 0);
            return 1;
        }
        
        if (strncmp(acl1->data[i].id.scheme,
                    acl2->data[i].id.scheme,
                    scheme1_len) != 0) {
            lua_pushboolean(L, 0);
            return 1;
        }
        
        int id1_len = strlen(acl1->data[i].id.id);
        int id2_len = strlen(acl2->data[i].id.id);
        
        if (id1_len != id2_len) {
            lua_pushboolean(L, 0);
            return 1;
        }
        
        if (strncmp(acl1->data[i].id.id,
                    acl2->data[i].id.id,
                    id1_len) != 0) {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

/***************** ACL end *****************/

static struct zk_global_wctx *
_zk_global_wctx_init(lua_State *L,
                     int zhref,
                     int cbref,
                     int internal_ctx_ref,
                     int user_ctx_ref)
{
    struct zk_global_wctx *wctx = (struct zk_global_wctx *)malloc(
        sizeof(struct zk_global_wctx));
    if (wctx == NULL) {
        luaL_error(L, "zookeep: out of memory");
    }

    say_debug("Setting global watcher. L=%p cbref=%d internal_ctx_ref=%d user_ctx_ref=%d", 
             L, cbref, internal_ctx_ref, user_ctx_ref);
    wctx->L = L;
    wctx->zhref = zhref;
    wctx->cbref = cbref;
    wctx->internal_ctx_ref= internal_ctx_ref;
    wctx->user_ctx_ref = user_ctx_ref;
    return wctx;
}

static void
_zk_global_wctx_free(lua_State *L,
                     struct zk_global_wctx *wctx)
{
    if (wctx == NULL) {
        return;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->zhref);
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->cbref);
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->internal_ctx_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->user_ctx_ref);
    free(wctx);
    return;
}

static struct zk_local_wctx *
_zk_local_wctx_init(lua_State *L,
                    int zhref,
                    int cbref,
                    int internal_ctx_ref,
                    int user_ctx_ref)
{
    struct zk_local_wctx *wctx =
        (struct zk_local_wctx *) malloc(sizeof(struct zk_local_wctx));
    if (wctx == NULL) {
        luaL_error(L, "zookeep: out of memory");
    }
    wctx->L = L;
    wctx->zhref = zhref;
    wctx->cbref = cbref;
    wctx->internal_ctx_ref = internal_ctx_ref;
    wctx->user_ctx_ref = user_ctx_ref;
    return wctx;
}

static void
_zk_local_wctx_free(lua_State *L,
                    struct zk_local_wctx *wctx)
{
    if (wctx == NULL) {
        return;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->zhref);
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->cbref);
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->internal_ctx_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, wctx->user_ctx_ref);
    free(wctx);
    return;
}

/**
 * initialize C clientid_t struct from lua table.
 **/
static clientid_t *
_zk_clientid_init(lua_State *L,
                  int index)
{
    size_t passwd_len = 0;
    const char *clientid_passwd = NULL;
    clientid_t *clientid = NULL;
    clientid = (clientid_t *) malloc(sizeof(clientid_t));
    if (clientid == NULL) {
        luaL_error(L, "zookeep: out of memory");
    }

    luaL_checktype(L, index, LUA_TTABLE);
    lua_getfield(L, index, "client_id");
    clientid->client_id =  luaL_checkint(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, index, "passwd");
    clientid_passwd = luaL_checklstring(L, -1, &passwd_len);
    lua_pop(L, 1);
    
    memset(clientid->passwd, 0, 16);
    memcpy(clientid->passwd, clientid_passwd, passwd_len);

    return clientid;
}

static void
_zk_clientid_free(clientid_t **clientid)
{
    if (*clientid != NULL) {
        free(*clientid);
        *clientid = NULL;
    }
}

static void
_zoo_handle_reinit(struct lua_zoo_handle *handle) 
{
    if (handle == NULL) {
        return;
    }
    
    if (handle->zh != NULL) {
        zookeeper_close(handle->zh);
        handle->zh = NULL;
    }

    handle->zh = zookeeper_init(handle->host, /* host */
                                NULL, /* watcher */
                                handle->recv_timeout, /* recv_timeout */
                                handle->client_id, /* clientid */
                                NULL, /* context */
                                handle->flags /* flags */);
    handle->prev_state = ZOO_NOTCONNECTED_STATE;

    if (handle->global_wctx != NULL) {
        zoo_set_watcher(handle->zh, watcher_dispatch);
        zoo_set_context(handle->zh, (void *) handle->global_wctx);
    }
}

/**
 * initialize a zookeeper handle.
 **/
static int
lua_zoo_init(lua_State *L)
{
    int top = lua_gettop(L);
    const char *host = NULL;
    size_t host_len = 0;
    int recv_timeout = 0;
    double reconnect_timeout = 1;
    clientid_t *clientid = NULL;
    int flags = 0;

    struct lua_zoo_handle *handle = (struct lua_zoo_handle *) lua_newuserdata(
        L, sizeof(struct lua_zoo_handle));
        
    luaL_getmetatable(L, ZOOKEEP_MT_NAME);
    lua_setmetatable(L, -2);

    host = luaL_checklstring(L, 1, &host_len);
    recv_timeout = luaL_checkint(L, 2);
    
    if (top >= 3 && !lua_isnil(L, 3)) {
        luaL_checktype(L, 3, LUA_TTABLE);
        clientid = _zk_clientid_init(L, 3);
    }
    
    if (top >= 4 && !lua_isnil(L, 4)) {
        flags = luaL_checkint(L, 4);
    }
    
    if (top >= 5 && !lua_isnil(L, 5)) {
        reconnect_timeout = luaL_checknumber(L, 5);
    }
    
    zoo_set_log_stream(stdout);
    handle->zh = NULL;
    handle->global_wctx = NULL;
    handle->connected_cond = NULL;
    handle->reconnect_timeout = reconnect_timeout;
    handle->client_id = clientid;
    handle->flags = flags;
    handle->recv_timeout = recv_timeout;
    handle->host = strdup(host);

    _zoo_handle_reinit(handle);
    return 1;
}

static int
lua_zoo_close(lua_State *L)
{
    int ret = 0;
    struct lua_zoo_handle *handle = luaL_checkudata(L, 1, ZOOKEEP_MT_NAME);
    if (handle == NULL) {
        return 0;
    }
    
    if (handle->zh != NULL) {
        ret = zookeeper_close(handle->zh);
        handle->zh = NULL;
    }
    
    if (handle->global_wctx != NULL) {
        _zk_global_wctx_free(L, handle->global_wctx);
        handle->global_wctx = NULL;
    }
    
    if (handle->connected_cond != NULL) {
        fiber_cond_delete(handle->connected_cond);
        handle->connected_cond = NULL;
    }

    if (handle->client_id != NULL) {
        _zk_clientid_free(&handle->client_id);
    }

    if (handle->host != NULL) {
        free(handle->host);
        handle->host = NULL;
    }
    
    lua_pushinteger(L, ret);
    return 1;
}

/**
 * return clientid_t of the current connection.
 **/
static int
lua_zoo_client_id(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle(L, 1);
    
    const clientid_t *clientid = zoo_client_id(handle->zh);
    lua_newtable(L);
    lua_pushstring(L, "client_id");
    lua_pushnumber(L, clientid->client_id);
    lua_settable(L, -3);
    lua_pushstring(L, "passwd");
    lua_pushstring(L, clientid->passwd);
    lua_settable(L, -3);
    return 1;
}

static int
lua_zoo_process(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle(L, 1);
    
    int fd = -1;
    int interest = 0;
    int coio_events = 0;
    int zoo_events = 0;
    int rc = ZOK;
    double timeout = 0;
    struct timeval tv;
    int state = 0;
        
    while (true) {
        fd = -1;
        interest = 0;
        coio_events = 0;
        zoo_events = 0;
        timeout = 0;
        
        rc = zookeeper_interest(handle->zh, &fd, &interest, &tv);
        say_info("zookeep: handle: %p; zookeeper_interest rc = %d", handle, rc);
        if (rc != ZOK) {
            say_crit(
                "zookeep: error while receiving zookeeper interest. rc = %d; fd = %d; state = %d",
                rc, fd, zoo_state(handle->zh));
            break;
        }
        
        if (fd != -1) {
            if (interest & ZOOKEEPER_READ) {
                coio_events |= COIO_READ;
            } else {
                coio_events &= ~COIO_READ;
            }
            if (interest & ZOOKEEPER_WRITE) {
                coio_events |= COIO_WRITE;
            } else {
                coio_events &= ~COIO_WRITE;
            }
            
            timeout = (double)(tv.tv_sec + (double) tv.tv_usec / 1000000.0);
            coio_events = coio_wait(fd, coio_events, timeout);
            
            if (fiber_is_cancelled()) {
                break;
            }
            
            if (coio_events == 0) {
                // timeout
                continue;
            }
            
            zoo_events = 0;
            if (coio_events & COIO_READ) {
                zoo_events |= ZOOKEEPER_READ;
            }
            if (coio_events & COIO_WRITE) {
                zoo_events |= ZOOKEEPER_WRITE;
            }
            rc = zookeeper_process(handle->zh, zoo_events);
            
            state = zoo_state(handle->zh);
            if (state != handle->prev_state) {
                if (state == ZOO_CONNECTED_STATE
                        && handle->connected_cond != NULL) {
                    fiber_cond_broadcast(handle->connected_cond);
                }
                handle->prev_state = state;
            }
        } else {
            say_warn(
                "zookeep: reconnecting in %.3fs", handle->reconnect_timeout);
            _zoo_handle_reinit(handle);
            fiber_sleep(handle->reconnect_timeout);
            continue;
        }
    }
    say_debug("zookeep: finished processing");
    return 0;
}

static int
lua_zoo_state(lua_State *L)
{
    int ret = 0;
    struct lua_zoo_handle *handle = _zk_check_zoo_handle(L, 1);
    
    ret = zoo_state(handle->zh);
    lua_pushinteger(L, ret);
    return 1;
}

static int
lua_zoo_wait_connected(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle(L, 1);
    
    double timeout = -1;
    if (!lua_isnil(L, 2)) {
        timeout = luaL_checknumber(L, 2);
    }
    
    if (zoo_state(handle->zh) == ZOO_CONNECTED_STATE) {
        return 0;
    }
    
    if (handle->connected_cond == NULL) {
        handle->connected_cond = fiber_cond_new();
    }
    int ret;
    if (timeout < 0) {
        ret = fiber_cond_wait(handle->connected_cond);
        if (ret == 0) {
            // wakeup
            return 0;
        }
        return luaL_error(L, "unexpected error");
    } else {
        ret = fiber_cond_wait_timeout(handle->connected_cond, timeout);
        if (ret == 0) {
            // wakeup
            return 0;
        } else if (ret == -1) {
            return luaL_error(L, "timeout");
        }
        
        return luaL_error(L, "unexpected error");
    }
}

static int
lua_zookeep_set_watcher(lua_State *L)
{
    int top = lua_gettop(L);
    struct zk_global_wctx *wctx;
    int zhref = 0;
    int cbref = 0;
    int internal_ctx_ref = 0;
    int user_ctx_ref = 0;
    int has_user_ctx = 0;
    
    struct lua_zoo_handle *handle = _zk_check_zoo_handle(L, 1);
    if (handle->global_wctx != NULL) {
        _zk_global_wctx_free(L,handle->global_wctx);
        handle->global_wctx = NULL;
    }
    if (top < 2 || lua_isnil(L, 2)) { /* lua watcher function */
        /* remove watcher */
        zoo_set_watcher(handle->zh, NULL);
        zoo_set_context(handle->zh, NULL);
        return 0;
    }
    luaL_checktype(L, 2, LUA_TFUNCTION);
    luaL_checktype(L, 3, LUA_TTABLE);  /* internal zookeep context */
    if (top > 3 && !lua_isnil(L, 4)) {
        has_user_ctx = 1; /* user's context */
    }
    
    lua_pushvalue(L, 1);
    zhref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 2);
    cbref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    internal_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if (has_user_ctx) {
        lua_pushvalue(L, 4);
        user_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    
    wctx = _zk_global_wctx_init(L, zhref, cbref,
                                internal_ctx_ref, user_ctx_ref);
    handle->global_wctx = wctx;
    
    zoo_set_watcher(handle->zh, watcher_dispatch);
    zoo_set_context(handle->zh, (void *) wctx);
    return 0;
}

static int
lua_zoo_set_log_level(lua_State *L)
{
    int level = luaL_checkint(L, -1);
    switch(level) {
        case ZOO_LOG_LEVEL_ERROR:
            zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
            break;
        case ZOO_LOG_LEVEL_WARN:
            zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
            break;
        case ZOO_LOG_LEVEL_INFO:
            zoo_set_debug_level(ZOO_LOG_LEVEL_INFO);
            break;
        case ZOO_LOG_LEVEL_DEBUG:
            zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
            break;
        default:
            return luaL_error(L, "unsupported log level specified");
    }
    return 0;
}

static struct zk_data_result *
_zk_data_result_init(lua_State *L)
{
    struct zk_data_result *cdata = NULL;
    cdata = (struct zk_data_result *) malloc(sizeof(struct zk_data_result));
    if (cdata == NULL) {
        luaL_error(L, "zookeep: out of memory");
        return NULL;
    }
    cdata->L = L;
    cdata->cond = fiber_cond_new();
    return cdata;
}

static void
_zk_data_result_free(struct zk_data_result *cdata)
{
    if (cdata == NULL) {
        return;
    }
    
    fiber_cond_delete(cdata->cond);
    free(cdata);
}

static int
_zk_handle_operation_result(lua_State *L,
                            struct zk_data_result *cdata,
                            int ret)
{
    if (cdata == NULL) {
        return 0;
    }
    
    if (ret != ZOK) {
        _zk_data_result_free(cdata);
        return luaL_error(L, zerror(ret));
    }
    
    ret = fiber_cond_wait(cdata->cond);
    int ret_count = cdata->ret_count;
    
    _zk_data_result_free(cdata);
    if (ret == 0) {
        // wakeup
        return ret_count;
    }
    return luaL_error(L, "unexpected error");
}

static int
_zk_parse_watch_flag(lua_State *L, int index) {
    if (lua_isnil(L, index)) {
        return 0;
    }
    
    if (lua_isboolean(L, index)) {
        return lua_toboolean(L, index);
    }
    
    return luaL_error(L, "watch must either a nil or boolean");
}

static int
lua_zoo_add_auth(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle(L, 1);
    
    size_t cert_len = 0;
    const char *scheme = NULL;
    const char *cert = NULL;
    
    scheme = luaL_checkstring(L, 2);
    cert = luaL_checklstring(L, 3, &cert_len);

    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_add_auth(handle->zh,
                           scheme,
                           cert,
                           cert_len,
                           _zk_void_cb,
                           cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_deterministic_conn_order(lua_State *L)
{
    int value = 0;
   
    if (!lua_isnil(L, 1) && lua_isboolean(L, 1)) {
        value = lua_toboolean(L, 1);
    } else {
        return luaL_error(L, "invalid argument: boolean is required");
    }
    
    zoo_deterministic_conn_order(value);
    return 0;
}

static int
lua_zoo_zerror(lua_State *L)
{
    int code = luaL_checkint(L, -1);
    const char *errstr = zerror(code);
    lua_pushstring(L, errstr);
    return 1;
}

static int
lua_zoo_create(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    
    const char *value = NULL;
    size_t value_len = 0;
    
    struct ACL_vector *zoo_acl;
    int flags = 0;

    path = luaL_checklstring(L, 2, &path_len);
    if (!lua_isnil(L, 3)) {
        value = luaL_checklstring(L, 3, &value_len);
    }
    zoo_acl = _zk_check_zoo_acl(L, 4);
    if (!lua_isnil(L, 5)) {
        flags = luaL_checkint(L, 5);
    }
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_acreate(handle->zh,
                          path,
                          value,
                          value_len,
                          zoo_acl,
                          flags,
                          _zk_string_cb,
                          cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_delete(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int version = -1;

    path = luaL_checklstring(L, 2, &path_len);
    if (!lua_isnil(L, 3)) {
        version = luaL_checkint(L, 3);
    }
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_adelete(handle->zh,
                          path,
                          version,
                          _zk_void_cb,
                          cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_exists(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int watch = 0;

    path = luaL_checklstring(L, 2, &path_len);
    watch = _zk_parse_watch_flag(L, 3);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aexists(handle->zh,
                          path,
                          watch,
                          _zk_stat_cb,
                          cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}


static int
lua_zoo_get(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int watch = 0;

    path = luaL_checklstring(L, 2, &path_len);
    watch = _zk_parse_watch_flag(L, 3);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aget(handle->zh,
                       path,
                       watch,
                       _zk_data_cb,
                       cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_set(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    
    const char *value = NULL;
    size_t value_len = 0;
    
    int version = -1;

    path = luaL_checklstring(L, 2, &path_len);
    value = luaL_checklstring(L, 3, &value_len);
    
    if (!lua_isnil(L, 4)) {
        version = luaL_checkint(L, 4);
    }
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aset(handle->zh,
                       path,
                       value,
                       value_len,
                       version,
                       _zk_stat_cb,
                       cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_get_children(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int watch = 0;

    path = luaL_checklstring(L, 2, &path_len);
    watch = _zk_parse_watch_flag(L, 3);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aget_children(handle->zh,
                                path,
                                watch,
                                _zk_strings_cb,
                                cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_get_children2(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int watch = 0;

    path = luaL_checklstring(L, 2, &path_len);
    watch = _zk_parse_watch_flag(L, 3);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aget_children2(handle->zh,
                                 path,
                                 watch,
                                 _zk_strings_stat_cb,
                                 cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_sync(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;

    path = luaL_checklstring(L, 2, &path_len);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_async(handle->zh,
                        path,
                        _zk_string_cb,
                        cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_wexists(lua_State *L)
{
    int top = lua_gettop(L);
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int zhref = 0;
    int cbref = 0;
    int internal_ctx_ref = 0;
    int user_ctx_ref = 0;
    int has_user_ctx = 0;
    struct zk_local_wctx *wctx;

    /* check arguments */
    path = luaL_checklstring(L, 2, &path_len);
    luaL_checktype(L, 3, LUA_TFUNCTION); /* lua watcher function */
    luaL_checktype(L, 4, LUA_TTABLE);  /* internal zookeep context */
    if (top > 4 && !lua_isnil(L, 5)) {
        has_user_ctx = 1; /* user's context */
    }
    
    /* saving references to context objects */
    lua_pushvalue(L, 1);
    zhref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    cbref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 4);
    internal_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if (has_user_ctx) {
        lua_pushvalue(L, 5);
        user_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    wctx = _zk_local_wctx_init(L, zhref, cbref,
                               internal_ctx_ref, user_ctx_ref);
    
    /* make request */
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_awexists(handle->zh,
                           path,
                           local_watcher_dispatch,
                           (void *) wctx,
                           _zk_stat_cb,
                           cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_wget(lua_State *L)
{
    int top = lua_gettop(L);
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int zhref = 0;
    int cbref = 0;
    int internal_ctx_ref = 0;
    int user_ctx_ref = 0;
    int has_user_ctx = 0;
    struct zk_local_wctx *wctx;

    /* check arguments */
    path = luaL_checklstring(L, 2, &path_len); /* path */
    luaL_checktype(L, 3, LUA_TFUNCTION); /* lua watcher function */
    luaL_checktype(L, 4, LUA_TTABLE);  /* internal zookeep context */
    if (top > 4 && !lua_isnil(L, 5)) {
        has_user_ctx = 1; /* user's context */
    }
    
    /* saving references to context objects */
    lua_pushvalue(L, 1);
    zhref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    cbref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 4);
    internal_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if (has_user_ctx) {
        lua_pushvalue(L, 5);
        user_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    wctx = _zk_local_wctx_init(L, zhref, cbref,
                               internal_ctx_ref, user_ctx_ref);
    
    /* make request */
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_awget(handle->zh,
                        path,
                        local_watcher_dispatch,
                        (void *) wctx,
                        _zk_data_cb,
                        cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_wget_children(lua_State *L)
{
    int top = lua_gettop(L);
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int zhref = 0;
    int cbref = 0;
    int internal_ctx_ref = 0;
    int user_ctx_ref = 0;
    int has_user_ctx = 0;
    struct zk_local_wctx *wctx;

    /* check arguments */
    path = luaL_checklstring(L, 2, &path_len); /* path */
    luaL_checktype(L, 3, LUA_TFUNCTION); /* lua watcher function */
    luaL_checktype(L, 4, LUA_TTABLE);  /* internal zookeep context */
    if (top > 4 && !lua_isnil(L, 5)) {
        has_user_ctx = 1; /* user's context */
    }
    
    /* saving references to context objects */
    lua_pushvalue(L, 1);
    zhref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    cbref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 4);
    internal_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if (has_user_ctx) {
        lua_pushvalue(L, 5);
        user_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    wctx = _zk_local_wctx_init(L, zhref, cbref,
                               internal_ctx_ref, user_ctx_ref);
    
    /* make request */
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_awget_children(handle->zh,
                                 path,
                                 local_watcher_dispatch,
                                 (void *) wctx,
                                 _zk_strings_cb,
                                 cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_wget_children2(lua_State *L)
{
    int top = lua_gettop(L);
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int zhref = 0;
    int cbref = 0;
    int internal_ctx_ref = 0;
    int user_ctx_ref = 0;
    int has_user_ctx = 0;
    struct zk_local_wctx *wctx;

    /* check arguments */
    path = luaL_checklstring(L, 2, &path_len); /* path */
    luaL_checktype(L, 3, LUA_TFUNCTION); /* lua watcher function */
    luaL_checktype(L, 4, LUA_TTABLE);  /* internal zookeep context */
    if (top > 4 && !lua_isnil(L, 5)) {
        has_user_ctx = 1; /* user's context */
    }
    
    /* saving references to context objects */
    lua_pushvalue(L, 1);
    zhref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 3);
    cbref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pushvalue(L, 4);
    internal_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    if (has_user_ctx) {
        lua_pushvalue(L, 5);
        user_ctx_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    wctx = _zk_local_wctx_init(L, zhref, cbref,
                               internal_ctx_ref, user_ctx_ref);
    
    /* make request */
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_awget_children2(handle->zh,
                                  path,
                                  local_watcher_dispatch,
                                  (void *) wctx,
                                  _zk_strings_stat_cb,
                                  cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_get_acl(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;

    path = luaL_checklstring(L, 2, &path_len);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aget_acl(handle->zh,
                           path,
                           _zk_acl_cb,
                           cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}

static int
lua_zoo_set_acl(lua_State *L)
{
    struct lua_zoo_handle *handle = _zk_check_zoo_handle_connected(L, 1);
    
    const char *path = NULL;
    size_t path_len = 0;
    int version = -1;
    struct ACL_vector *zoo_acl;
    
    path = luaL_checklstring(L, 2, &path_len);
    if (!lua_isnil(L, 3)) {
        version = luaL_checkint(L, 3);
    }
    zoo_acl = _zk_check_zoo_acl(L, 4);
    
    struct zk_data_result *cdata = _zk_data_result_init(L);
    int ret = zoo_aset_acl(handle->zh,
                           path,
                           version,
                           zoo_acl,
                           _zk_void_cb,
                           cdata);
    return _zk_handle_operation_result(L, cdata, ret);
}


#define _zk_register_constant(s)\
    lua_pushstring(L, #s);\
    lua_pushnumber(L, s);\
    lua_settable(L, -3);
    
#define _zk_register_constant_name(name, s)\
    lua_pushstring(L, name);\
    lua_pushnumber(L, s);\
    lua_settable(L, -3);


LUA_API int
luaopen_zookeeper_driver(lua_State *L)
{
    /*** ACL ***/
    static const struct luaL_Reg acl_methods[] = {
        {"totable",    lua_zoo_acl_totable},
        {"__eq",       lua_zoo_acl_eq},
        {"__tostring", lua_zoo_acl_tostring},
        {"__gc",       lua_zoo_destroy_acl_list},
        {NULL, NULL}
    };
    
    luaL_newmetatable(L, ZOOKEEP_ACL_LIST_MT_NAME);
    lua_pushvalue(L, -1);
    luaL_register(L, NULL, acl_methods);
    lua_setfield(L, -2, "__index");
    lua_pushstring(L, ZOOKEEP_ACL_LIST_MT_NAME);
    lua_setfield(L, -2, "__metatable");
    lua_pop(L, 1);
    
    /*** zookeep ***/
    static const struct luaL_Reg lua_zookeep_lib[] = {
        {"build_acl_list", lua_zoo_build_acl_list},
        
        {"init",                     lua_zoo_init},
        {"close",                    lua_zoo_close},
        {"client_id",                lua_zoo_client_id},
        {"process",                  lua_zoo_process},
        {"state",                    lua_zoo_state},
        {"wait_connected",           lua_zoo_wait_connected},
        {"set_watcher",              lua_zookeep_set_watcher},
        {"add_auth",                 lua_zoo_add_auth},
        {"deterministic_conn_order", lua_zoo_deterministic_conn_order},
        {"zerror",                   lua_zoo_zerror},
        {"set_log_level",            lua_zoo_set_log_level},
        
        /* operations methods */
        {"create",         lua_zoo_create},
        {"delete",         lua_zoo_delete},
        {"exists",         lua_zoo_exists},
        {"get",            lua_zoo_get},
        {"set",            lua_zoo_set},
        {"get_children",   lua_zoo_get_children},
        {"get_children2",  lua_zoo_get_children2},
        {"sync",           lua_zoo_sync},
        {"wexists",        lua_zoo_wexists},
        {"wget",           lua_zoo_wget},
        {"wget_children",  lua_zoo_wget_children},
        {"wget_children2", lua_zoo_wget_children2},
        {"get_acl",        lua_zoo_get_acl},
        {"set_acl",        lua_zoo_set_acl},
        {NULL, NULL}
    };

    luaL_newmetatable(L, ZOOKEEP_MT_NAME);
    luaL_register(L, NULL, lua_zookeep_lib);

    /**
     * Client errors.
     **/
    lua_newtable(L);
    _zk_register_constant(ZOK);
    _zk_register_constant(ZSYSTEMERROR);
    _zk_register_constant(ZRUNTIMEINCONSISTENCY);
    _zk_register_constant(ZCONNECTIONLOSS);
    _zk_register_constant(ZMARSHALLINGERROR);
    _zk_register_constant(ZUNIMPLEMENTED);
    _zk_register_constant(ZOPERATIONTIMEOUT);
    _zk_register_constant(ZBADARGUMENTS);
    _zk_register_constant(ZINVALIDSTATE);
    lua_setfield(L, -2, "errors");

    /**
     * API errors.
     **/
    lua_newtable(L);
    _zk_register_constant(ZAPIERROR);
    _zk_register_constant(ZNONODE);
    _zk_register_constant(ZNOAUTH);
    _zk_register_constant(ZBADVERSION);
    _zk_register_constant(ZNOCHILDRENFOREPHEMERALS);
    _zk_register_constant(ZNODEEXISTS);
    _zk_register_constant(ZNOTEMPTY);
    _zk_register_constant(ZSESSIONEXPIRED);
    _zk_register_constant(ZINVALIDCALLBACK);
    _zk_register_constant(ZINVALIDACL);
    _zk_register_constant(ZAUTHFAILED);
    _zk_register_constant(ZCLOSING);
    _zk_register_constant(ZNOTHING);
    _zk_register_constant(ZSESSIONMOVED);
    lua_setfield(L, -2, "api_errors");

    /**
     * ACL Constants.
     **/
    lua_newtable(L);
    _zk_register_constant_name("READ", ZOO_PERM_READ);
    _zk_register_constant_name("WRITE", ZOO_PERM_WRITE);
    _zk_register_constant_name("DELETE", ZOO_PERM_DELETE);
    _zk_register_constant_name("ADMIN", ZOO_PERM_ADMIN);
    _zk_register_constant_name("ALL", ZOO_PERM_ALL);
    lua_setfield(L, -2, "permissions");

    /**
     * Debug Levels.
     **/
    lua_newtable(L);
    _zk_register_constant_name("ERROR", ZOO_LOG_LEVEL_ERROR);
    _zk_register_constant_name("WARN", ZOO_LOG_LEVEL_WARN);
    _zk_register_constant_name("INFO", ZOO_LOG_LEVEL_INFO);
    _zk_register_constant_name("DEBUG", ZOO_LOG_LEVEL_DEBUG);
    lua_setfield(L, -2, "log_level");

    /**
     * Create Flags.
     **/
    lua_newtable(L);
    _zk_register_constant_name("EPHEMERAL", ZOO_EPHEMERAL);
    _zk_register_constant_name("SEQUENCE", ZOO_SEQUENCE);
    lua_setfield(L, -2, "create_flags");

    /**
     * State Constants.
     **/
    lua_newtable(L);
    _zk_register_constant_name("EXPIRED_SESSION", ZOO_EXPIRED_SESSION_STATE);
    _zk_register_constant_name("AUTH_FAILED", ZOO_AUTH_FAILED_STATE);
    _zk_register_constant_name("CONNECTING", ZOO_CONNECTING_STATE);
    _zk_register_constant_name("ASSOCIATING", ZOO_ASSOCIATING_STATE);
    _zk_register_constant_name("CONNECTED", ZOO_CONNECTED_STATE);
    _zk_register_constant_name("READONLY", ZOO_READONLY_STATE);
    _zk_register_constant_name("NOTCONNECTED", ZOO_NOTCONNECTED_STATE);
    lua_setfield(L, -2, "states");
    

    /**
     * Watch Types.
     **/
    lua_newtable(L);
    _zk_register_constant_name("CREATED", ZOO_CREATED_EVENT);
    _zk_register_constant_name("DELETED", ZOO_DELETED_EVENT);
    _zk_register_constant_name("CHANGED", ZOO_CHANGED_EVENT);
    _zk_register_constant_name("CHILD", ZOO_CHILD_EVENT);
    _zk_register_constant_name("SESSION", ZOO_SESSION_EVENT);
    _zk_register_constant_name("NOTWATCHING", ZOO_NOTWATCHING_EVENT);
    lua_setfield(L, -2, "watch_types");

    return 1;
}

#undef _zk_register_constant
