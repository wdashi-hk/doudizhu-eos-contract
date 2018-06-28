#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>
#include <eosio.token/eosio.token.hpp>

using namespace eosio;

class doudizhu : public eosio::contract {
  public:
      using contract::contract;

      struct poker{
    	  uint8_t suits;					//花色
    	  uint8_t point;					//点数
    	  poker(uint8_t _suits, uint8_t _point){
    		  suits = _suits;
    		  point = _point;
    	  }
      };
      struct pokerData{
    	  uint8_t type;						//牌型
    	  uint8_t suits;					//花色
    	  uint8_t point;					//点数
    	  uint8_t len;						//牌个数
    	  pokerData(uint8_t _type, uint8_t _suits, uint8_t _point, uint8_t _len){
    		  type = _type;
    		  suits = _suits;
    		  point = _point;
    		  len = _len;
    	  }
      };
      struct planeData{
    	  vector<poker> p1;
    	  vector<poker> p2;
    	  uint8_t k;
    	  planeData(vector<poker> _p1, vector<poker> _p2, uint8_t _k){
    		  p1 = _p1;
    		  p2 = _p2;
    		  k = _k;
    	  }
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
    	  chupai_data(uint8_t _index, pokerData _data){
    		  player_index = _index;
    		  data = _data;
    	  }
      };
      struct game{
    	  uint64_t game_id;						//游戏ID号，唯一索引
    	  game_status status;					//游戏状态
    	  vector<player> players;				//游戏玩家
    	  vector<poker> poker_array_dz;			//地主的底牌
    	  uint8_t turn_index;					//出牌、抢地主的人
    	  vector<qdz_data> qdz_data_array;		//保存抢地主的数据
    	  vector<chupai_data> chupai_data_array;//保存一轮出牌数据

    	  uint8_t game_level;					//游戏级别，1分 2分 3分，炸弹翻倍。默认0分
    	  uint64_t token_amount;				//抵押的币总数
    	  uint8_t base_token;					//基数
    	  uint8_t max_level;					//最大的倍数
      };

      const static uint8_t MAX_PLAYER_NUM = 3;					//最多3个玩家
      account_name feeaccount = N(doudizhu);					//收取手续费的账号
      account_name adminaccount = N(doudizhu);					//管理员的账号
      uint8_t fee = 100;										//手续费，默认1%


      vector<game> games_all;						//存储所有游戏
      uint8_t max_games_can_join;					//最多可以加入的游戏数量
      uint8_t min_games_can_join;					//最少可以加入的游戏数量

      vector<uint64_t> games_id_can_join;			//存储所有可以加入的游戏id
      vector<uint64_t> games_id_end;				//存储所有结束的游戏id

      //创建游戏
      void create(account_name name, uint64_t & gameid, uint8_t base, uint8_t maxlevel){
  		require_auth(name);
  		eosio_assert(name == adminaccount, "can't do it!");
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
  		g.base_token = base;
  		g.max_level = maxlevel;
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
      void autojoin(account_name name, uint8_t base, uint8_t maxlevel){
    	  require_auth(name);
    	  uint64_t size = games_id_can_join.size();
    	  if(size < min_games_can_join){  //新建一局游戏再加入
    		  uint64_t gameid;
    		  create(name, gameid, base, maxlevel);
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
    		  player p = g.players.at(playerindex);
    		  p.is_win = false;
    		  if(playerindex == 0){
    			  g.players.at(1).is_win = true;
    			  g.players.at(2).is_win = true;
    		  }
    		  else if(playerindex == 1){
    			  g.players.at(0).is_win = true;
    			  g.players.at(2).is_win = true;
    		  }
    		  else if(playerindex == 2){
    			  g.players.at(1).is_win = true;
    			  g.players.at(0).is_win = true;
    		  }
    		  g.status = E_END;
    		  games_id_end.push_back(gameid);
    		  endgame(gameid);
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
		  if(g.chupai_data_array.size()){
			  olddata = g.chupai_data_array.back().data;
		  }
		  if(vs(pd, olddata)){
			  player p = g.players.at(playerindex);
			  eosio_assert(deletepoker(pokerarray, p.poker_array), "error poker! can't delete");
			  g.turn_index = (g.turn_index + 1) % MAX_PLAYER_NUM;
			  g.chupai_data_array.push_back(chupai_data(playerindex, pd));
			  if(p.poker_array.size() == 0){  //出完牌了，游戏结束
				  endgame(gameid);
				  return;
			  }
			  if(g.chupai_data_array.size() == MAX_PLAYER_NUM){
				  g.turn_index = playerindex;
			  }
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
		  g.chupai_data_array.push_back(chupai_data(playerindex, pd));
		  if(g.chupai_data_array.size() == MAX_PLAYER_NUM){
			  if(g.chupai_data_array.at(MAX_PLAYER_NUM - 2).data.point){
				  g.turn_index = g.chupai_data_array.at(MAX_PLAYER_NUM - 2).player_index;
			  }
			  else{
				  g.turn_index = g.chupai_data_array.at(0).player_index;
			  }
		  }
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
    	  pokerarray.push_back(poker(14,0));
    	  pokerarray.push_back(poker(14,1));
    	  for(int i = 1; i <= 13; ++i){
    		  for(int j = 0; j <= 3; ++j){
    			  pokerarray.push_back(poker(i,j));
    		  }
    	  }

    	  for(int i = 0; i < 54; ++i){
    		  uint8_t r = random() % 54;
    		  swappoker(pokerarray.at(i), pokerarray.at(r));
    	  }

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
      void endgame(uint64_t gameid){
    	  game g = games_all.at(gameid);
    	  for(int i = 0; i < g.players.size(); ++i){
    		  player p = g.players.at(i);
    		  if(p.is_win){
    			  uint8_t plus = 1;
    			  if(p.is_dz) plus *= 2;
    			  uint64_t amount = g.token_amount/3 + g.game_level * g.base_token * plus;
    			  uint64_t feeamount = amount * fee / 10000;  //扣除手续费
    			  transferSYS(feeaccount, p.player_name, amount - feeamount);
    		  }
    		  else{
    			  uint8_t plus = 1;
    			  if(p.is_dz) plus *= 2;
    			  uint64_t amount = g.token_amount/3 - g.game_level * g.base_token * plus;
    			  transferSYS(feeaccount, p.player_name, amount);
    		  }
    	  }
      }
      //判断是否在游戏中
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

      void swappoker(poker & p1, poker & p2){
		  uint8_t suits, point;
		  suits = p1.suits;
		  point = p1.point;
		  p1.suits = p2.suits;
		  p1.point = p2.point;
		  p2.suits = suits;
		  p2.point = point;
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

      //给牌排序
      vector<poker> sortpoker(vector<poker> pokerarray){
    	 int8_t n = pokerarray.size();
 	     for(int i = 1; i < n; i++){
 	         if(pokerarray.at(i).point > pokerarray.at(i - 1).point){
 	        	 int8_t temppoint = pokerarray.at(i).point;
 	        	 int8_t tempsuits = pokerarray.at(i).suits;
 	        	 int j;
 	        	 for(j = i-1; j >= 0 && pokerarray.at(i).point < temppoint; j--){
 	        		 pokerarray.at(j+1).point = pokerarray.at(j).point;
 	        		 pokerarray.at(j+1).suits = pokerarray.at(j).suits;
 	        	 }
 	        	 pokerarray.at(j+1).point = temppoint;
 	        	 pokerarray.at(j+1).suits = tempsuits;
 	         }
 	     }
    	 return pokerarray;
      }
      //截断飞机
      planeData cutplane(vector<poker> pokerarray){
    	  int k = 0;
    	  int index = 0;
    	  vector<poker> temparray;
    	  vector<poker> leftarray;

    	  for(int i = 0; i < pokerarray.size() - 2; ++i){
    		  if(pokerarray.at(i).point == pokerarray.at(i+2).point){
    			  k++;
    			  if(k == 1) index = i;  //保存第一个三张牌的第一张牌下标
    		  }
    	  }

    	  for(int i = 0; i < pokerarray.size(); ++i){
    		  if(i >= index && i != (index +(3*k))){
    			  temparray.push_back(pokerarray.at(i));  //将3张牌拿出来放到一个新数组
    		  }
    		  else{
    			  leftarray.push_back(pokerarray.at(i));  //将非3张牌拿出来放到一个新数组
    		  }
    	  }
    	  return planeData(temparray, leftarray, k);
      }

      //判断是否为飞机，是的话返回点数
      int plane(vector<poker> pokerarray, uint8_t type){
    	  planeData planedata = cutplane(pokerarray);
    	  uint8_t len = pokerarray.size();
    	  int ret = 0;
    	  switch(planedata.k){
    	  case 1:
    		  if(len == 4 || len == 5)
    			  ret = order(planedata.p1, 9, 3);
    		  else
    			  return 0;
    		  break;
    	  case 2:
    		  if(len == 8 || len == 10)
    			  ret = order(planedata.p1, 9, 3);
    		  else
    			  return 0;
    		  break;
    	  case 3:
    		  if(len == 12 || len == 15)
    			  ret = order(planedata.p1, 9, 3);
    		  else
    			  return 0;
    		  break;
    	  case 4:
    		  if(len == 16 || len == 20)
    			  ret = order(planedata.p1, 9, 3);
    		  else
    			  return 0;
    		  break;
    	  case 5:
    		  if(len == 20)
    			  ret = order(planedata.p1, 9, 3);
    		  else
    			  return 0;
    		  break;
    	  }

    	  switch(type){
    	  case 10:  //3带1
    		  if(ret)
    			  return planedata.p1.at(0).point;
    		  else
    			  return 0;
    		  break;
    	  case 11:	//3带1对
    		  if(ret){
    			  for(int i = 0; i < planedata.p2.size() - 2; i+=2){
    				  if(planedata.p2.at(i).point != planedata.p2.at(i+1).point)
    					  return 0;
    			  }
    			  return planedata.p1.at(0).point;
    		  }
    		  break;
    	  }
    	  return ret;
      }
      //判断是否为顺子，type：7代表顺子 8代表连对 9代表三连对  plus：自增
      bool order(vector<poker> pokerarray, uint8_t type, uint8_t plus){
    	  for(int i = 0; i < pokerarray.size() - 1; i+=plus){
    		  if(pokerarray.at(i).point < 13){ //最大的牌在3-A之间
    			  if(type == 7){  //判断是否顺子
    				  if(pokerarray.at(i).point != pokerarray.at(i+1).point + 1){
    					  return false;
    				  }
    			  }
    			  else if(type == 8){ //判断是否连对
    				  if(pokerarray.at(i).point != pokerarray.at(i+1).point &&
    						  pokerarray.at(i+1).point != pokerarray.at(i+2).point + 1){
    					  return false;
    				  }
    			  }
    			  else if(type == 9){ //判断是否三连对
    				  if(pokerarray.size() == 3 &&
    						  pokerarray.at(i).point != pokerarray.at(i+2).point){
    					  return false;
    				  }
    				  else if(pokerarray.at(i).point != pokerarray.at(i+2).point &&
    						  pokerarray.at(i).point != pokerarray.at(i+3).point + 1){
    					  return false;
    				  }
    			  }
    		  }
    		  else{  //3条2
    			  if(pokerarray.size() == 3 &&
    					  pokerarray.at(i).point != pokerarray.at(i+2).point){
    				  return true;
    			  }
    			  else
    				  return false;
    		  }
    	  }
    	  return true;
      }
      //判断出牌的类型
      pokerData poker2data(vector<poker> pokerarray){
    	  pokerData pd;
    	  vector<poker> s = sortpoker(pokerarray);
    	  uint8_t len = s.size();
    	  if(len == 0){
    		  return pd;
    	  }
    	  else if(len == 1){
    		  return pokerData(1, s.at(0).suits, s.at(0).point, 1);
    	  }
    	  else if(len == 2){
    		  if(s.at(0).point == 14 && s.at(1).point == 14 && s.at(0).suits == 1 && s.at(0).suits == 0){  //王炸
    			  return pokerData(13, s.at(0).suits, s.at(0).point, 2);
    		  }
    		  else if(s.at(0).point == s.at(1).point){ //对子
    			  return pokerData(2, s.at(0).suits, s.at(0).point, 2);
    		  }
    		  else{
    			  return pd;
    		  }
    	  }
    	  else if(len == 3){
    		  if(s.at(0).point == s.at(1).point && s.at(1).point == s.at(2).point){ //三条
    			  return pokerData(3, s.at(0).suits, s.at(0).point, 3);
    		  }
    		  else{
    			  return pd;
    		  }
    	  }
    	  else if(len == 4){ //炸弹或者飞机
    		  if(s.at(0).point == s.at(1).point && s.at(1).point == s.at(2).point && s.at(2).point == s.at(3).point){ //炸弹
    			  return pokerData(12, s.at(0).suits, s.at(0).point, 4);
    		  }
    		  else{
    			  int ret = plane(s, 10); //3带1
    			  if(ret)
    				  return pokerData(4, s.at(0).suits, ret, 4);
    		  }
    	  }
    	  else if(len == 5){ //3带2 或者是 顺子
    		  pokerData p = plane(s, 11); //3带2
    		  if(p.type == 0){
    			  bool ret = order(s, 7, 1);
    			  if(ret)
    				  return pokerData(5, s.at(0).suits, s.at(0).point, 5);
    		  }
    		  return p;
    	  }
    	  else if(len == 6){ //4带2 或者 顺子/连对/三连对
    		  if(s.at(0).point == s.at(3).point){
    			  return pokerData(6, s.at(0).suits, s.at(0).point, 6);
    		  }
    		  else if(s.at(1).point == s.at(4).point){
    			  return pokerData(6, s.at(1).suits, s.at(1).point, 6);
    		  }
    		  else if(s.at(2).point == s.at(5).point){
    			  return pokerData(6, s.at(2).suits, s.at(2).point, 6);
    		  }
    		  else{ //顺子/连对/三连对
    			  bool pd1 = order(s, 7, 1);
    			  bool pd2 = order(s, 8, 2);
    			  bool pd3 = order(s, 9, 3);

    			  if(pd1){
    				  return pokerData(7, s.at(0).suits, s.at(0).point, 6);
    			  }
    			  else if(pd2){
    				  return pokerData(8, s.at(0).suits, s.at(0).point, 6);
    			  }
    			  else if(pd3){
    				  return pokerData(9, s.at(0).suits, s.at(0).point, 6);
    			  }
    		  }
    	  }
    	  else if(len == 7){ //只能是顺子
    		  bool ret = order(s, 7, 1);
    		  if(ret)
				  return pokerData(7, s.at(0).suits, s.at(0).point, 7);
    	  }
    	  else if(len == 8){ //飞机或者顺子
    		  int p = plane(s, 10);
			  bool pd1 = order(s, 7, 1);
			  bool pd2 = order(s, 8, 2);

			  if(p){
				  return pokerData(10, s.at(0).suits, p, 8);
			  }
			  else if(pd1){
				  return pokerData(7, s.at(0).suits, s.at(0).point, 8);
			  }
			  else if(pd2){
				  return pokerData(8, s.at(0).suits, s.at(0).point, 8);
			  }
    	  }
    	  else if(len == 9){  //顺子或三连对
			  bool pd1 = order(s, 7, 1);
			  bool pd2 = order(s, 9, 3);
			  if(pd1){
				  return pokerData(7, s.at(0).suits, s.at(0).point, 9);
			  }
			  else if(pd2){
				  return pokerData(9, s.at(0).suits, s.at(0).point, 9);
			  }
    	  }
    	  else if(len == 10){
    		  int p = plane(s, 11);
			  bool pd1 = order(s, 7, 1);
			  bool pd2 = order(s, 8, 2);

			  if(p){
				  return pokerData(11, s.at(0).suits, p, 10);
			  }
			  else if(pd1){
				  return pokerData(7, s.at(0).suits, s.at(0).point, 10);
			  }
			  else if(pd2){
				  return pokerData(8, s.at(0).suits, s.at(0).point, 10);
			  }
    	  }
    	  else if(len == 11){
    		  bool ret = order(s, 7, 1);
    		  if(ret)
    			  return pokerData(7, s.at(0).suits, s.at(0).point, 11);
    	  }
    	  else if(len == 12){
    		  int p = plane(s, 10);
			  bool pd1 = order(s, 7, 1);
			  bool pd2 = order(s, 8, 2);
			  bool pd3 = order(s, 9, 3);

			  if(p){
				  return pokerData(10, s.at(0).suits, p, 12);
			  }
			  else if(pd1){
				  return pokerData(7, s.at(0).suits, s.at(0).point, 12);
			  }
			  else if(pd2){
				  return pokerData(8, s.at(0).suits, s.at(0).point, 12);
			  }
			  else if(pd3){
				  return pokerData(9, s.at(0).suits, s.at(0).point, 12);
			  }
    	  }
    	  else if(len == 14){
    		  bool ret = order(s, 8, 2);
    		  if(ret)
				  return pokerData(8, s.at(0).suits, s.at(0).point, 14);
    	  }
    	  else if(len == 15){
    		  int p = plane(s, 11);
			  bool pd1 = order(s, 9, 3);

			  if(p){
				  return pokerData(11, s.at(0).suits, p, 15);
			  }
			  else if(pd1){
				  return pokerData(9, s.at(0).suits, s.at(0).point, 15);
			  }
    	  }
    	  else if(len == 16){
    		  int p = plane(s, 10);
			  bool pd1 = order(s, 8, 2);

			  if(p){
				  return pokerData(10, s.at(0).suits, p, 16);
			  }
			  else if(pd1){
				  return pokerData(8, s.at(0).suits, s.at(0).point, 16);
			  }
    	  }
    	  else if(len == 18){
			  bool pd1 = order(s, 8, 2);
			  bool pd2 = order(s, 9, 3);

			  if(pd1){
				  return pokerData(8, s.at(0).suits, s.at(0).point, 18);
			  }
			  else if(pd2){
				  return pokerData(9, s.at(0).suits, s.at(0).point, 18);
			  }
    	  }
    	  else if(len == 20){
    		  int p1 = plane(s, 10);
    		  int p2 = plane(s, 11);
			  bool pd1 = order(s, 8, 2);

			  if(p1){
				  return pokerData(10, s.at(0).suits, p1, 20);
			  }
			  else if(pd1){
				  return pokerData(8, s.at(0).suits, s.at(0).point, 20);
			  }
			  else if(p2){
				  return pokerData(11, s.at(0).suits, p2, 20);
			  }
    	  }
    	  return pd;
      }

      bool vs(pokerData newdata, pokerData olddata){
    	  if(newdata.type == olddata.type){  //牌型一样的时候
    		  if(newdata.len != olddata.len)
    			  return false;
    		  else{
    			  if(newdata.len == 1 && newdata.point == 14 && olddata.point == 14){
    				  return newdata.suits > olddata.suits;
    			  }
    			  return newdata.point > olddata.point;
    		  }
    	  }
    	  else{
    		  return (newdata.type > 11 && olddata.type != 13);  //选择的牌型为王炸和炸弹且场面牌不为王炸
    	  }
    	  return false;
      }

      bool deletepoker(vector<poker> deletearray, vector<poker> & fromarray){
    	  int len = fromarray.size();
    	  for(int i = 0; i < deletearray.size(); ++i){
    		  poker p = deletearray.at(i);
    		  for(vector<poker>::iterator itr = fromarray.begin(); itr != fromarray.end(); ++ itr){
    			  if(itr->point == p.point && itr->suits == p.suits){
    				  fromarray.erase(itr);
    			  }
    		  }
    	  }
    	  return (fromarray.size() == len - deletearray.size());
      }

      //转账SYS
      bool transferSYS(account_name _from, account_name _to, uint64_t _amount){
    	  extended_asset amount(100,S(4,SYS));
    	  amount.contract = N(eosio.token);
    	  currency::inline_transfer(_from, _to, amount);
      }

      //获取SYS帐户余额
      asset getSYSBalance(account_name name){
    	  eosio::token t(N(eosio.token));
    	  const auto sym_name = eosio::symbol_type(S(4,SYS)).name();
    	  const auto my_balance = t.get_balance(N(myaccount), sym_name );
    	  return my_balance;
      }

};

EOSIO_ABI( doudizhu, (create)(join)(autojoin)(leave)(qdz)(chupai)(buchu) )

