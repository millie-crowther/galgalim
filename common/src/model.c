#include "model.h"

// bool instance_exists(CassSession * redis_context, const char * game_id, const char * instance_id){
//     redisReply * reply = redisCommand(redis_context, "SISMEMBER instances %s", instance_id);
//     bool is_found = reply->integer;
//     freeReplyObject(reply);
//     return is_found;
// }
