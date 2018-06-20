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
    	  bool is_ready;					//是否准备好
    	  bool is_dz;						//是否是地主
    	  bool is_win;						//是否赢
    	  uint64_t join_time;				//加入游戏的时间
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
    	  uint8_t game_level;				//游戏级别，1分 2分 3分，炸弹翻倍。默认0分
      };

      const static uint8_t MAX_PLAYER_NUM = 3;		//最多3个玩家

      vector<game> games_all;						//存储所有游戏
      uint8_t max_games_can_join;					//最多可以加入的游戏数量
      uint8_t min_games_can_join;					//最少可以加入的游戏数量

      vector<uint64_t> games_id_can_join;			//存储所有可以加入的游戏id
      vector<uint64_t> games_id_full;				//存储所有不可加入的游戏id
      vector<uint64_t> games_id_on_going;			//存储所有进行中的游戏id
      vector<uint64_t> games_id_end;				//存储所有结束的游戏id

      //创建游戏
      void create(account_name name, uint64_t & gameid){
  		require_auth(name);
  		uint8_t s = games_id_can_join.size();
  		if(s == max_games_can_join){
  			print("max games can join, can not create");
  			return;
  		}
  		else if(s == max_games_can_join - 1){
  			clearplayernotready();
  		}

  		game g;
  		g.game_id = games_all.size();
  		games_all.push_back(g);
  		games_id_can_join.push_back(g.game_id);
  		gameid = g.game_id;
      }
      //加入游戏
      void join(account_name name, uint64_t gameid){
  		require_auth(name);
  		game g = games_all.at(gameid);
  		eosio_assert(g.players.size() < MAX_PLAYER_NUM, "max players, can not join");
  		for(int i = 0; i < g.players.size(); ++i){
  			player p = g.players.at(i);
  			eosio_assert(p.player_name != name, "already in game, can not join");
  		}
  		player p;
  		p.player_name = name;
  		p.join_time = current_time();
  		g.players.push_back(p);

  		if(g.players.size() == MAX_PLAYER_NUM){
  			g.status = E_FULL;
  			//remove game id from games_id_can_join
  			for(vector<uint64_t>::iterator itr = games_id_can_join.begin(); itr != games_id_can_join.end(); ++itr){
  				if(*itr == gameid){
  					games_id_can_join.erase(itr);
  				}
  			}
  		}
      }
      //自动加入
      void autojoin(account_name name){
    	  uint64_t size = games_id_can_join.size();
    	  if(size < min_games_can_join){  //新建一局游戏再加入
    		  uint64_t gameid;
    		  create(name, gameid);
    		  join(name, gameid);
    	  }
    	  else{	//随机加入
    		  uint64_t autoindex = random() % size;
    		  uint64_t gameid = games_id_can_join.at(autoindex);
    		  join(name, gameid);
    	  }
      }
      //离开游戏
      void leave(account_name name, uint64_t gameid){
    	  game g = games_all.at(gameid);
    	  if(g.status >= E_QDZ){  //强制退出游戏，扣分

    	  }

      }
      //准备好了
      void getready(account_name name, uint64_t gameid){}
      //取消准备
      void cancleready(account_name name, uint64_t gameid){}
      //抢地主
      void qdz(account_name name, uint64_t gameid){}
      //出牌
      void chupai(account_name name, uint64_t gameid){}

      //config
      void config(){}

      //查询单局游戏信息
      game gameInfo(uint64_t gameid){
    	  return {0};
      }
      //查询多局游戏信息
      vector<game> gameInfos(uint64_t ids[]){
    	  return {0};
      }
      //查询用户当前的牌
      vector<poker> playerpokers(account_name player, uint64_t gameid){
    	  return {0};
      }
      //查询用户玩过的游戏
      vector<game> playergames(account_name player){
    	  return {0};
      }


  private:
      //随机函数
      uint64_t random(){
    	  return current_time();
      }
      //随机发牌
      vector<poker> randompoker(){
    	  return {0};
      }
      //长期不准备，强制让用户离开
      bool clearplayernotready(){
    	  return false;
      }

};

EOSIO_ABI( doudizhu, (create)(join)(autojoin)(leave)(qdz)(chupai) )

