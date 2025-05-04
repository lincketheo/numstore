#include "services/services.h"
#include "services/var_create.h"
#include "services/var_retr.h"

services
services_create (services_params params)
{
  return (services){
    .vcreate = var_create_create ((vcreate_params){
        .hm = params.vhm,
    }),
    .vretr = var_retr_create ((vrtr_params){
        .hm = params.vhm,
    }),
  };
}
