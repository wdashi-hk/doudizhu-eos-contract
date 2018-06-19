#include <eosiolib/eosio.hpp>
using namespace eosio;

class doudizhu : public eosio::contract {
  public:
      using contract::contract;

      struct poker{};
      struct pokerData{};
      struct player{};
      struct game{};

      void create(){}
      void join(){}
      void autojoin(){}
      void

};

EOSIO_ABI( doudizhu, () )
