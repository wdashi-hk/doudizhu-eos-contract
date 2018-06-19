#include <eosiolib/eosio.hpp>
using namespace eosio;

class doudizhu : public eosio::contract {
  public:
      using contract::contract;

      struct poker{
    	  uint8_t suits;					//花色
    	  uint8_t point;					//点数
      };
      struct pokerData{
    	  std::string poker_str;			//牌
    	  uint8_t suits;					//花色
    	  uint8_t point;					//点数
      };
      struct player{
    	  account_name player_name;			//用户id
    	  vector<poker>	poker_array;		//用户的牌
    	  bool isDZ;						//是否是地主
    	  bool isWin;						//是否赢
      };
      enum game_status{
    	  E_CAN_JOIN,						//初始状态，可加入游戏
		  E_FULL,							//满人，不能加入
		  E_QDZ,							//抢地主阶段
		  E_ONGOING,						//游戏进行中
		  E_END,							//游戏结束
      };
      struct game{
    	  uint64_t game_id;					//游戏ID号，唯一索引
    	  game_status status;				//游戏状态
    	  vector<player> players;			//游戏玩家
    	  vector<poker> poker_array_dz;		//地主的底牌
    	  uint8_t game_level;				//游戏级别，1分 2分 3分，炸弹翻倍
      };

      const static uint8_t MAX_PLAYER_NUM = 3;		//最多3个玩家

      vector<game> games_all;						//存储所有游戏

      vector<uint64_t> games_id_can_join;			//存储所有可以加入的游戏
      vector<uint64_t> games_id_full;				//存储所有不可加入的游戏
      vector<uint64_t> games_id_on_going;			//存储所有进行中的游戏
      vector<uint64_t> games_id_end;				//存储所有结束的游戏

      //创建游戏
      void create(account_name name){}
      //加入游戏
      void join(account_name name, uint64_t gameid){}
      //自动加入
      void autojoin(account_name name){}
      //离开游戏
      void leave(account_name name, uint64_t gameid){}
      //抢地主
      void qdz(account_name name, uint64_t gameid){}
      //出牌
      void chupai(account_name name, uint64_t gameid){}

      //查询单局游戏信息
      game gameInfo(uint64_t gameid){
    	  return NULL;
      }
      //查询多局游戏信息
      vector<game> gameInfos(uint64_t ids[]){
    	  return NULL;
      }
      //查询用户当前的牌
      vector<poker> playerpokers(account_name player, uint64_t gameid){
    	  return NULL;
      }
      //查询用户玩过的游戏
      vector<game> playergames(account_name player){
    	  return NULL;
      }


  private:
      //随机函数
      uint64_t random(){
    	  return current_time();
      }
      //随机发牌
      vector<poker> randompoker(){
    	  return NULL;
      }
};

EOSIO_ABI( doudizhu, (create)(join)(autojoin)(leave)(qdz)(chupai) )


void doudizhu::create(account_name name){

}
