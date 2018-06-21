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
    	  uint8_t type;						//牌型
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
      struct qdz_data{
    	  uint8_t player_index;
    	  uint8_t level;
    	  qdz_data(uint8_t _index, uint8_t _level){
    		  player_index = _index;
    		  level = _level;
    	  }
      };
      struct chupai_data{
    	  uint8_t player_index;
    	  pokerData data;
    	  bool buchu;
    	  chupai_data(uint8_t _index, pokerData _data, bool _buchu){
    		  player_index = _index;
    		  data = _data;
    		  buchu = _buchu;
    	  }
      };
      struct game{
    	  uint64_t game_id;						//游戏ID号，唯一索引
    	  game_status status;					//游戏状态
    	  vector<player> players;				//游戏玩家
    	  vector<poker> poker_array_dz;			//地主的底牌
    	  uint8_t game_level;					//游戏级别，1分 2分 3分，炸弹翻倍。默认0分
    	  uint8_t turn_index;					//出牌、抢地主的人
    	  vector<qdz_data> qdz_data_array;		//保存抢地主的数据
    	  vector<chupai_data> chupai_data_array;//保存一轮出牌数据
      };

      const static uint8_t MAX_PLAYER_NUM = 3;		//最多3个玩家

      vector<game> games_all;						//存储所有游戏
      uint8_t max_games_can_join;					//最多可以加入的游戏数量
      uint8_t min_games_can_join;					//最少可以加入的游戏数量

      vector<uint64_t> games_id_can_join;			//存储所有可以加入的游戏id
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
  					break;
  				}
  			}
  		}
      }
      //自动加入
      void autojoin(account_name name){
    	  require_auth(name);
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
    	  require_auth(name);
    	  game g = games_all.at(gameid);
    	  int playerindex;
		  eosio_assert(playeringame(name, gameid, playerindex), "player not in this game!");
    	  if(g.status >= E_QDZ){  //强制退出游戏，扣分，游戏结束
    		  //TODO: kou fen
    		  g.status = E_END;
    		  games_id_end.push_back(gameid);
    	  }
    	  else{
    		  for(vector<player>::iterator itr = g.players.begin(); itr != g.players.end(); ++itr){
    			  if(itr->player_name == name){
    				  g.players.erase(itr);
    				  g.status = E_CAN_JOIN;
    		  		  for(vector<uint64_t>::iterator itr = games_id_can_join.begin(); itr != games_id_can_join.end(); ++itr){
    		  		    if(*itr == gameid){
    		  				games_id_can_join.erase(itr);
    		  				break;
    		  			}
    		  		  }
    		  		  games_id_can_join.push_back(gameid);
    				  break;
    			  }
    		  }
    	  }
      }
      //准备好了
      void getready(account_name name, uint64_t gameid){
    	  require_auth(name);
    	  game g = games_all.at(gameid);
    	  int playerindex;
		  eosio_assert(playeringame(name, gameid, playerindex), "player not in this game!");
    	  eosio_assert(g.status < E_QDZ, "game is going, can't get ready");
		  player p = g.players.at(playerindex);
		  p.is_ready = true;
    	  if(g.status == E_FULL){
    		  g.status = E_QDZ;
        	  for(int i = 0; i < g.players.size(); ++i){
        		  player pp = g.players.at(i);
        		  if(pp.is_ready == false){
        			  g.status = E_FULL;
        			  break;
        		  }
        	  }
        	  if(g.status == E_QDZ){  //开始抢地主
        		  begingame(gameid);
        	  }
    	  }
      }
      //取消准备
      void cancelready(account_name name, uint64_t gameid){
          require_auth(name);
    	  game g = games_all.at(gameid);
    	  int playerindex;
		  eosio_assert(playeringame(name, gameid, playerindex), "player not in this game!");
    	  eosio_assert(g.status < E_QDZ, "game is going, can't cancel ready");
		  player p = g.players.at(playerindex);
		  p.is_ready = false;
		  g.status = E_FULL;
      }
      //抢地主，0分是不抢
      void qdz(account_name name, uint64_t gameid, uint8_t level){
    	  require_auth(name);
    	  game g = games_all.at(gameid);
    	  int playerindex;
		  eosio_assert(level < 3, "not valid level!");
		  eosio_assert(playeringame(name, gameid, playerindex), "player not in this game!");
		  eosio_assert(g.status == E_QDZ, "current status is not qdz");
		  g.qdz_data_array.push_back(qdz_data(playerindex, level));
		  g.turn_index = (playerindex + 1) % MAX_PLAYER_NUM;

		  uint8_t dz_index = 0;
		  for(int i = 0; i < g.qdz_data_array.size(); ++i){
			  qdz_data d = g.qdz_data_array.at(i);
			  if(g.game_level > d.level){
				  g.game_level = d.level;
				  dz_index = d.player_index;
			  }

			  if(d.level == 3){ //有人抢了3分，结束抢地主
				  player p = g.players.at(d.level);
				  p.is_dz = true;
				  g.game_level = d.level;
				  g.turn_index = d.player_index;
				  g.status = E_ONGOING;
				  p.poker_array.push_back(g.poker_array_dz.at(0));
				  p.poker_array.push_back(g.poker_array_dz.at(1));
				  p.poker_array.push_back(g.poker_array_dz.at(2));
				  break;
			  }
			  else if(i == MAX_PLAYER_NUM - 1){  //已经抢完一轮了，检查谁是地主
				  if(g.game_level == 0){ //过分，重新开始游戏
					  g.status = E_QDZ;
					  begingame(gameid);
				  }
				  else{
					  player p = g.players.at(dz_index);
					  p.is_dz = true;
					  g.turn_index = dz_index;
					  g.status = E_ONGOING;
					  p.poker_array.push_back(g.poker_array_dz.at(0));
					  p.poker_array.push_back(g.poker_array_dz.at(1));
					  p.poker_array.push_back(g.poker_array_dz.at(2));
				  }
				  break;
			  }
		  }
      }
      //出牌,pokerstr用逗号分割牌，如："1_0,13_1"
      void chupai(account_name name, uint64_t gameid, std::string pokerstr){
    	  require_auth(name);
    	  game g = games_all.at(gameid);
    	  int playerindex;
		  eosio_assert(playeringame(name, gameid, playerindex), "player not in this game!");
		  vector<poker> pokerarray = str2poker(pokerstr);
		  eosio_assert(pokerarray.size(), "error poker!");
		  pokerData pd = poker2data(pokerarray);
		  pokerData olddata;
		  for(int i = 0; i < g.poker_data_array.size(); ++i){
			  pokerData d = g.poker_data_array.at(i);
			  if(d.point != 0) olddata = d;
		  }
		  if(vs(pd, olddata)){
			  player p = g.players.at(playerindex);
			  eosio_assert(deletepoker(pokerarray, p.poker_array), "error poker! can't delete");
			  g.turn_index = (g.turn_index + 1) % MAX_PLAYER_NUM;
			  if(p.poker_array.size() == 0){  //出完牌了，游戏结束
				  endgame(gameid, p);
				  return;
			  }
			  checkround(gameid);
		  }
      }

      //不出牌
      void buchu(account_name name, uint64_t gameid){
    	  require_auth(name);
    	  game g = games_all.at(gameid);
    	  int playerindex;
		  eosio_assert(playeringame(name, gameid, playerindex), "player not in this game!");
		  g.turn_index = (g.turn_index + 1) % MAX_PLAYER_NUM;
		  pokerData pd;
		  g.poker_data_array.push_back(pd);
		  checkround(gameid);
      }

      //config
      void config(){}

//      //查询单局游戏信息
//      game gameInfo(uint64_t gameid){
//
//      }
//      //查询多局游戏信息
//      vector<game> gameInfos(uint64_t ids[]){
//
//      }
//      //查询用户当前的牌
//      vector<poker> playerpokers(account_name player, uint64_t gameid){
//
//      }
//      //查询用户玩过的游戏
//      vector<game> playergames(account_name player){
//
//      }


  private:
      //随机函数
      uint64_t random(){
    	  return current_time();
      }
      //随机发牌
      vector<poker> randompoker(){
    	  vector<poker> pokerarray;
    	  return pokerarray;
      }
      //长期不准备，强制让用户离开
      bool clearplayernotready(){
    	  return false;
      }
      //开始游戏
      void begingame(uint64_t gameid){
    	  game g = games_all.at(gameid);
    	  vector<poker> pokerarray = randompoker();
    	  //发牌
    	  for(int i = 0; i < 51; ++i){
    		  uint8_t index = i % MAX_PLAYER_NUM;
    		  g.players.at(index).poker_array.push_back(pokerarray.at(i));
    	  }
    	  g.poker_array_dz.push_back(pokerarray.at(51));
    	  g.poker_array_dz.push_back(pokerarray.at(52));
    	  g.poker_array_dz.push_back(pokerarray.at(53));
    	  //随机选个人抢地主
		  uint8_t turnindex = random() % MAX_PLAYER_NUM;
		  g.turn_index = turnindex;
      }
      //结束游戏
      void endgame(uint64_t gameid, player winplayer){

      }
      bool playeringame(account_name user,uint64_t gameid, int & playerindex){
    	  game g = games_all.at(gameid);
    	  playerindex = -1;
    	  for(int i = 0; i < g.players.size(); ++i){
    		  player p = g.players.at(i);
    		  if(p.player_name == user){
    			  playerindex = i;
    			  return true;
    		  }
    	  }
    	  return false;
      }

      void splitString(const std::string& s, std::vector<std::string>& v, const std::string& c){
        std::string::size_type pos1, pos2;
        pos2 = s.find(c);
        pos1 = 0;
        while(std::string::npos != pos2)
        {
          v.push_back(s.substr(pos1, pos2-pos1));

          pos1 = pos2 + c.size();
          pos2 = s.find(c, pos1);
        }
        if(pos1 != s.length())
          v.push_back(s.substr(pos1));
      }

      vector<poker> str2poker(std::string pokerstr){
    	  vector<poker> pokerarray;
    	  vector<std::string> pokerstrarray;
    	  splitString(pokerstr, pokerstrarray, ",");
    	  for(int i = 0; i < pokerstrarray.size(); ++i){
    		  std::string s = pokerstrarray.at(i);
    		  std::string::size_type pos = s.find("_");
    		  if(pos > 0 && pos < s.length() - 1){
        		  poker p;
        		  p.point = atoll(s.substr(0, pos).c_str()) ;
        		  p.suits = atoll(s.substr(pos + 1, 1).c_str());
        		  pokerarray.push_back(p);
    		  }
    	  }
    	  return pokerarray;
      }

      pokerData poker2data(vector<poker> pokerarray){
    	  pokerData pd;
    	  return pd;
      }

      bool vs(pokerData newdata, pokerData olddata){
    	  return false;
      }

      bool deletepoker(vector<poker> deletearray, vector<poker> fromarray){
    	  return false;
      }
      //检查一轮出牌
      void checkround(uint64_t gameid){
    	  game g = games_all.at(gameid);
    	  for(int i = 0; i < g.players.size(); ++i){
    		  player p = g.players.at(i);

    	  }
      }

};

EOSIO_ABI( doudizhu, (create)(join)(autojoin)(leave)(qdz)(chupai) )

