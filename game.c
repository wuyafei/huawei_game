
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct Card{
	char* color;
	char point;
};

int on_server_msg(int len, const char* buf){
	printf("received msg from server: %s\n", buf);
	return 0;
}

char* substr(char *dst, char *src, int len, int m, int n){
	// m is the start position
	// n is the length of substr
	char *p = src;
	char *q = dst;
	p += m;
	while(n--)
		*(q++) = *(p++);
	*(q++) = '\0';
	//printf("%s ",dst);
	return dst;
}

int strpchr(char *src, int len, char c, int p){
	int i = p;
	for(i=p;i<len;i++){
		if(c == src[i])
			return i;
	}
	return -1;
}
int card2num(char c, char p)//convert card to an integer
{	//card are coded by integer 0-51, where A:0, 2:1, 3:2...10:9, J:10, Q=11, K = 12
	//SPADES:0~12	HEARTS:13~25	CLUBS:26~38	DIAMONDS:39~51
	int i = 0, j = 0;
	//i = (c=='S')?0:((c=='H')?1:((c=='C')?2:3));
	if (c=='S') i = 0;
	else if (c=='H') i = 1;
		else if (c=='C') i = 2;
			else if (c=='D') i = 3;
				else printf("color error: c=%c !\n", c);
	if (p=='A') j = 0;
	else if (p=='J') j = 10;
		else if (p=='Q') j = 11;
			else if (p=='K') j = 12;
				else if (p=='1') j = 9;
					else if (p>='2'&&p<='9') j = p-'1';
						else printf("point error: p=%c !\n", p);
	return i*13+j;
}

int check_hold_cards(int a, int b){
	//return 1 for call or check
	//return 0 for fold
	if(a%13==0 && b%13==0 || a%13==12 && b%13==12 || a%13==0 && b%13==11 || a%13==11 && b%13==0 || a%13==0 && b%13==12 || a%13==12 && b%13==0 || a%13==11 && b%13==11 || a%13==10 && b%13==10 || a%13==9 && b%13==9 || a%13==8 && b%13==8 || a%13==7 && b%13==7 || a%13==6 && b%13==6)
		return 1;
	else
		return 0;
}

int check_cards(int* cards, int n){
	int a[7];  //origin
	int b[7];  //point
	int i,j;
	for(i=0;i<n;i++){
		a[i]=cards[i];
		b[i]=cards[i]%13;
	}
	for(i=0;i<n;i++){
		int k =i;
		for(j=i+1;j<n;j++){
			if(b[j]<b[k])
				k=j;
		}
		if(k!=i){
			int tmp = b[k];
			b[k] = b[i];
			b[i] = tmp;
		}
	}
	//two pairs or three-of-a-kind or full-house or four-of-a-kind
	int cnt=0;
	for(i=0;i<n-1;i++){
		if(b[i]%13==b[i+1]%13)
			cnt++;
	}
	if(cnt>1)
		return cnt-1;

	//strait
	if(b[0]%13+1==b[1]%13 && b[1]%13+1==b[2]%13 && b[2]%13+1==b[3]%13 && b[3]%13+1==b[4]%13)
		return 2;
	if(n>=6 && b[1]%13+1==b[2]%13 && b[2]%13+1==b[3]%13 && b[3]%13+1==b[4]%13 && b[4]%13+1==b[5]%13)
		return 2;
	if(n==7 && b[5]%13+1==b[6]%13 && b[2]%13+1==b[3]%13 && b[3]%13+1==b[4]%13 && b[4]%13+1==b[5]%13)
		return 2;

	for(i=0;i<n;i++){
		int k =i;
		for(j=i+1;j<n;j++){
			if(a[j]<a[k])
				k=j;
		}
		if(k!=i){
			int tmp = a[k];
			a[k] = a[i];
			a[i] = tmp;
		}
	}
	
	//flush
	if(a[0]/13 == a[4]/13)
		return 2;
	if(n>=6 && a[1]/13 == a[5]/13)
		return 2;
	if(n==7 && a[2]/13 == a[6]/13)
		return 2;

		//strait flush
	if(a[0]/13==a[4]/13 && a[0]%13+1==a[1]%13 && a[1]%13+1==a[2]%13 && a[2]%13+1==a[3]%13 && a[3]%13+1==a[4]%13)
		return 3;
	if(n>=6 && a[1]/13 == a[5]/13 && a[1]%13+1==a[2]%13 && a[2]%13+1==a[3]%13 && a[3]%13+1==a[4]%13 && a[4]%13+1==a[5]%13)
		return 3;
	if(n==7 && a[2]/13 == a[6]/13 && a[5]%13+1==a[6]%13 && a[2]%13+1==a[3]%13 && a[3]%13+1==a[4]%13 && a[4]%13+1==a[5]%13)
		return 3;

	return 0;
}


void strnmv(char *s, int len, int n)//move string: s[0~len-n] <- s[n~len]
{
	int i = 0;
	for (i = n; i< len; i ++)
		s[i-n] = s[i];
	s[len-n] = '\0';
}


int main(int argc, char *argv[]){
	//check arguments	
	if(argc != 6){
		printf("usage: ./%s server_ip server_port my_ip, my_port my_id\n",argv[0]);
		return -1;
	}

	//get arguments
	in_addr_t server_ip = inet_addr(argv[1]);
	in_port_t server_port = htons(atoi(argv[2]));
	in_addr_t my_ip = inet_addr(argv[3]);
	in_port_t my_port = htons(atoi(argv[4]));
	int my_id = atoi(argv[5]);

	//construct a socket
	int my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(my_socket < 0){
		printf("initialize socket failed!\n");
		return -1;
	}

	//set socket option for reuse address
	int reuse_addr = 1;
	setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr));

	//bind ip and port to socket
	struct sockaddr_in my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = my_ip;
	my_addr.sin_port = my_port;
	if(bind(my_socket, (struct sockaddr*)&my_addr, sizeof(my_addr))){
		printf("bind address to socket failed!\n");
		return -1;
	}

	//connect to server
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = server_ip;
	server_addr.sin_port = server_port;
	while(connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
		usleep(100*1000); //sleep 100ms and try again
	}
	printf("connected to gameserver\n");

	//register to gameserver
	char reg_msg[50] = {'\0'};
	snprintf(reg_msg, sizeof(reg_msg)-1, "reg: %d %s need_notify \n", my_id, "dusheng");
	send(my_socket, reg_msg, strlen(reg_msg)+1, 0);
	printf("register in game\n");

	//receive gameserver's msg and starting game
	int mycard[7]={0}, money[8]={0}, jetton[8]={0};
	char buf[4096] = {'\0'};
	int len = 0, ii = 0, tt = 0, sta = 0, end = 0;
	int game_num = 0, b_id, bb_id, sb_id, b_jetton, bb_jetton, sb_jetton, b_money, bb_money, sb_money;

	char id_buf[10], jetton_buf[10], money_buf[10];
	int player_num=0;
	// play game start
	while(1){
		if (buf[0]=='\0')
			len = recv(my_socket, buf, sizeof(buf)-1, 0);buf[len]='\0';

		if(strncmp(buf, "game-over ", 10) == 0)
			break;
		game_num ++;	printf("start play game: %d\n",game_num);
		printf("0\n");printf("%s", buf);
		//handle seat-info-msg
			//button
		sta = strpchr(buf, len, '\n', 0) + 1;
		sta = strpchr(buf, len, ':', sta) + 2;
                end = strpchr(buf, len, ' ', sta);
		b_id = atoi(substr(id_buf, buf, len ,sta, end-sta));
		sta = end + 1;
		end = strpchr(buf, len, ' ', sta);
		b_jetton = atoi(substr(jetton_buf, buf, len ,sta, end-sta));
		sta = end + 1;
		end = strpchr(buf, len, ' ', sta);
		b_money = atoi(substr(money_buf, buf, len ,sta, end-sta));
		//small blind
		sta = end + 2;
		sta = strpchr(buf, len, ':', sta) + 2;
		end = strpchr(buf, len, ' ', sta);
                sb_id = atoi(substr(id_buf, buf, len ,sta, end-sta));
                sta = end + 1;
                end = strpchr(buf, len, ' ', sta);
                sb_jetton = atoi(substr(jetton_buf, buf, len ,sta, end-sta));
                sta = end + 1;
                end = strpchr(buf, len, ' ', sta);
                sb_money = atoi(substr(money_buf, buf, len ,sta, end-sta));
		if (buf[end+2]!='/')
		{
			
		}
		else player_num = 2;

		//big blind
		//
		end = strpchr(buf, len, '/', 5)+6;//to read out entire the seat message
		sta = end + 1;
		strnmv(buf, len, sta);
		len = len - sta;

		//must be blind-msg
		if (buf[0]=='\0')
			len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';
		printf("1\n");printf("%s\n", buf);

		sta = strpchr(buf, len, '\n', 0) + 1;
		end = strpchr(buf, len, ':', sta);
		char pid_buf[10];
		int sb_pid = atoi(substr(pid_buf, buf, len, sta, end-sta));
		sta = end + 2;
		end = strpchr(buf, len, ' ', sta);
		char bet_buf[20];
		int sb_bet = atoi(substr(bet_buf, buf, len, sta, end-sta));
		printf("small blind: %d, %d\n", sb_pid, sb_bet);

		end = strpchr(buf, len, '/', 6)+7;// to read out the entire blind message
                sta = end + 1;
                strnmv(buf, len, sta);
                len = len - sta;

		//must be hold-cards-msg
		if (buf[0]=='\0')
			len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';
		printf("2\n");printf("%s\n", buf);
		sta = strpchr(buf, len, '\n', 0) + 1;
		end = strpchr(buf, len, ' ', sta);
		mycard[0]=card2num(buf[sta], buf[end+1]);//color and point stored by int
		//char col_1[10];
		//struct Card card_1 = {substr(col_1, buf, len, sta, end-sta), buf[end+1]};
		sta = strpchr(buf, len, '\n', end) + 1;
		end = strpchr(buf, len, ' ', sta);
		mycard[1]=card2num(buf[sta], buf[end+1]);
		//char col_2[10];
		//struct Card card_2 = {substr(col_2, buf, len, sta, end-sta), buf[end+1]};
		//printf("card_1: %d, card_2: %d\n", mycard[0], mycard[1]);
		int b_go_on = check_hold_cards(mycard[0], mycard[1]);
		printf("card1: %d, card2: %d, go on: %d\n", mycard[0], mycard[1], b_go_on);
		
		end = strpchr(buf, len, '/', 5)+6;
                sta = end + 1;
                strnmv(buf, len, sta);
                len = len - sta;

		//inquire-msg for hold round bet
		while(1){
			printf("3\n");
			if (buf[0]=='\0')
				len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';
			
			if (strncmp(buf,"inquire",7)==0) // inquire 
			{
				printf("%s\n", buf);
				char act_msg[20];
				if(b_go_on == 1 || player_num == 2)
					snprintf(act_msg, sizeof(act_msg)-1, "check \n");
				else
					snprintf(act_msg, sizeof(act_msg)-1, "fold \n");
				send(my_socket, act_msg, strlen(act_msg)+1, 0);

				end = strpchr(buf, len, '/', 8)+9;
                		sta = end + 1;
                		strnmv(buf, len, sta);
				len = len - sta;
			}
			else if ( strncmp(buf,"notify",6)==0)// I have take fold or all_in
			{
				printf("%s\n", buf);
				end = strpchr(buf, len, '/', 7)+8;
                                sta = end + 1;
                                strnmv(buf, len, sta);
                                len = len - sta;
			}
			else// end round bet
			{
				break;
			}
			
		}
		
		//flop-msg
		if (strncmp(buf,"pot-win",7)!=0)//2 or more players alive
		{
			printf("4\n");printf("%s\n", buf);
			sta = strpchr(buf, len, '\n', 0) + 1;
			end = strpchr(buf, len, ' ', sta);
			mycard[2]=card2num(buf[sta], buf[end+1]);

			sta = strpchr(buf, len, '\n', end) + 1;
			end = strpchr(buf, len, ' ', sta);
			mycard[3]=card2num(buf[sta], buf[end+1]);

			sta = strpchr(buf, len, '\n', end) + 1;
			end = strpchr(buf, len, ' ', sta);
			mycard[4]=card2num(buf[sta], buf[end+1]);

			b_go_on = check_cards(mycard, 5);
			printf("go on: %d\n", b_go_on);

			end = strpchr(buf, len, '/', 5)+6;
			sta = end + 1;
			strnmv(buf, len, sta);
			len = len - sta;

			//inquire-msg for flop round bet
			while(1){
				printf("5\n");
				if (buf[0]=='\0')
						len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';
                		if (strncmp(buf,"inquire",7)==0)
	                        {
	                                printf("%s\n", buf);
					if(b_go_on<=1)
	                                	send(my_socket, "check \n", 8, 0);
					else if(b_go_on==2)
		                               	send(my_socket, "raise 1000 \n", 13, 0);
					else 
	                                	send(my_socket, "all_in \n", 9, 0);
	
	                                end = strpchr(buf, len, '/', 8)+9;
	                                sta = end + 1;
	                                strnmv(buf, len, sta);
	                                len = len - sta;
	                        }
	                        else if ( strncmp(buf,"notify",6)==0)
	                        {
	                                printf("%s\n", buf);
	                                end = strpchr(buf, len, '/', 7)+8;
	                                sta = end + 1;
	                                strnmv(buf, len, sta);
	                                len = len - sta;
	                        }
        	                else// round bet end
        	                {
        	                        break;
        	                }
			}

			//turn-msg
			if (strncmp(buf,"pot-win",7)!=0)//2 or more players alive
			{
				printf("6\n");printf("%s\n", buf);
                		sta = strpchr(buf, len, '\n', 0) + 1;
                		end = strpchr(buf, len, ' ', sta);
                		mycard[5]=card2num(buf[sta], buf[end+1]);

					b_go_on = check_cards(mycard, 6);
					printf("go on: %d\n", b_go_on);

				end = strpchr(buf, len, '/', 5)+6;
                		sta = end + 1;
                		strnmv(buf, len, sta);
                		len = len - sta;

				//inquire-msg for turn round bet
                		while(1){
					printf("7\n");
					if (buf[0]=='\0')
                 			       len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';

					if (strncmp(buf,"inquire",7)==0)
		                        {
        		                        printf("%s\n", buf);
									if(b_go_on==1)
	                                	send(my_socket, "check \n", 8, 0);
									else if(b_go_on==2)
	                                	send(my_socket, "raise 1000 \n", 13, 0);
									else if(b_go_on==3)
	                                	send(my_socket, "all_in \n", 9, 0);
									else
	                                	send(my_socket, "fold \n", 7, 0);

                        		        end = strpchr(buf, len, '/', 8)+9;
                        		        sta = end + 1;
						strnmv(buf, len, sta);
                                		len = len - sta;
                        		}
                        		else if ( strncmp(buf,"notify",6)==0)
                        		{
                                		printf("%s\n", buf);
                                		end = strpchr(buf, len, '/', 7)+8;
                                		sta = end + 1;
                                		strnmv(buf, len, sta);
                                		len = len - sta;
                        		}
                        		else// round bet end
                        		{
                                		break;
                        		}
                		}

                		//river-msg
				if (strncmp(buf,"pot-win",7)!=0)//2 or more players alive
				{
					printf("8\n");printf("%s\n", buf);
        		        	sta = strpchr(buf, len, '\n', 0) + 1;
        		        	end = strpchr(buf, len, ' ', sta);
        		        	mycard[6]=card2num(buf[sta], buf[end+1]);

							b_go_on = check_cards(mycard, 7);
							printf("go on: %d\n", b_go_on);

					end = strpchr(buf, len, '/', 6)+7;
                			sta = end + 1;
                			strnmv(buf, len, sta);
                			len = len - sta;

					//inquire-msg river round bet
                			while(1){
                        			printf("9\n");
						if (buf[0]=='\0')
							len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';
						if (strncmp(buf,"inquire",7)==0)
			                        {
                			                printf("%s\n", buf);
											if(b_go_on==1)
													send(my_socket, "check \n", 8, 0);
											else if(b_go_on==2)
													send(my_socket, "raise 1000 \n", 13, 0);
											else if(b_go_on==3)
													send(my_socket, "all_in \n", 9, 0);
											else
													send(my_socket, "fold \n", 7, 0);
                                			end = strpchr(buf, len, '/', 8)+9;
                                			sta = end + 1;
                                			strnmv(buf, len, sta);
                                			len = len - sta;
                        			}
                        			else if ( strncmp(buf,"notify",6)==0)
                        			{
                                			printf("%s\n", buf);
                                			end = strpchr(buf, len, '/', 7)+8;
                                			sta = end + 1;
                                			strnmv(buf, len, sta);
                                			len = len - sta;
                        			}
                        			else
                        			{
                                			break;
                        			}

					}

					//showdown-msg
					if (strncmp(buf,"pot-win",7)!=0)//two or more players alive
					{
						printf("10\n");printf("%s\n", buf);
						sta = 0;
						for (ii = 0;ii < 8; ii++)//the common card is invalid
                					sta = strpchr(buf, len, '\n', sta) + 1;
		
		
						end = strpchr(buf, len, '/', sta)+10;
                				sta = end + 1;
                				strnmv(buf, len, sta);
               					len = len - sta;
					}

				}
			}
		}
		//pot-win-msg
		if (buf[0]=='\0')
			len = recv(my_socket, buf,sizeof(buf)-1, 0);buf[len]='\0';
		
		printf("11\n");printf("%s\n", buf);
                sta = strpchr(buf, len, '\n', 0) + 1;
                end = strpchr(buf, len, ' ', sta);//pot-win 0~8 possible records

                //sta = strpchr(buf, len, '\n', end) + 1;
                //end = strpchr(buf, len, ' ', sta);
		
		end = strpchr(buf, len, '/', 8)+9;
                sta = end + 1;
                strnmv(buf, len, sta);
                len = len - sta;

		//if (game_num>2) break;

	}
	close(my_socket);
	return 0;
}
