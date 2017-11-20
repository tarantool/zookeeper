#include <tarantool/module.h>
#include <lauxlib.h>

#include <zookeeper/zookeeper.h>

#define ZOOKEEP_MT_NAME "__zookeeper_handle"
#define ZOOKEEP_ACL_LIST_MT_NAME "__zookeeper_acl_list"


struct zk_global_wctx {
    lua_State *L;
    int zhref;
    int cbref;
    int internal_ctx_ref;
    int user_ctx_ref;
};


struct zk_local_wctx {
    lua_State *L;
    int zhref;
    int cbref;
    int internal_ctx_ref;
    int user_ctx_ref;
};


struct lua_zoo_handle {
    zhandle_t *zh;
    struct zk_global_wctx *global_wctx; /* global watcher context */
    double reconnect_timeout;
    struct fiber_cond *connected_cond;
    int prev_state;
};


struct zk_data_result {
    lua_State *L;
    int ret_count;
    
    struct fiber_cond *cond;
};
