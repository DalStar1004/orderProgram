#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include "/usr/include/mysql/mysql.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyACM0"
#define _POSIX_SOURCE 1 / POSIX compliant source
#define FALSE 0
#define TRUE 1
#define BUF_MAX 25

void eror_handling(char* mesage);
const char* commaValue(long n);

int mainMenu();
int seeMenu(char menuType[]);
int addMenuToList(char name[], int price, char menu[]);
int seeShopList();
int orderMenu();

typedef struct _shopListNode {
	char name[50];
	int price;
	int amount;
	struct _shopListNode* next;
} shopListNode;

shopListNode* shopListHead = NULL;

typedef struct _menusNode {                         
	char name[50];
	int price;
	struct _menusNode* next;
} menusNode;

char sql_query[100] = "";
MYSQL* conn, connection;
MYSQL_RES* sql_result;
MYSQL_ROW sql_row;

char *server = "localhost";
char *user = "orderManager";
char *password = "1004";
char *database = "lotteria";

int serialCommunication(){
	int fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
	if (fd <0) { perror(MODEMDEVICE); exit(-1); } 
	struct termios tio;
	memset(&tio, 0, sizeof(tio));
	tcgetattr(fd,&tio);
 
 
	tio.c_cflag |= BAUDRATE;
	tio.c_cflag |= CS8;
	tio.c_cflag |= CLOCAL;
	tio.c_cflag |= CREAD;
	tio.c_iflag = IGNPAR;

	tcsetattr(fd,TCSANOW,&tio);
	sleep(3);
	write(fd, "o", 1);
 
	close(fd);
	return 0;
}
const char* commaValue(long n) {
	static char comma_str[64];
	char str[64];
	int idx, len, cidx = 0;
	int mod;
	snprintf(str,sizeof(str),"%ld", n);
	len = strlen(str);
	mod = len % 3;

	for (idx = 0; idx < len; idx++) {
		if (idx != 0 && (idx) % 3 == mod) {
			comma_str[cidx++] = ',';
		}
		comma_str[cidx++] = str[idx];
	}

	comma_str[cidx] = 0x00;
	return comma_str;
}

int main(void) {
	system("Title : Lotteria Order Program");

	mysql_init(&connection);
	conn = mysql_real_connect(&connection, server, user, password, database, 3306, (char*)NULL, 0);

	return mainMenu();
}

int mainMenu() {
	int menuSelect = 0;
	printf("\n======[lotteria Order Program]======\n");
	printf("1. View hamburger menu\n");
	printf("2. View beverage menu\n");
	printf("3. View dessert menu\n");
	printf("4. View shopping basket\n");
	printf("5. Order\n");
	printf("0. Exit\n");
	printf("====================================\n");
	printf("Select Number: ");
	scanf("%d", &menuSelect);
	
	switch (menuSelect) {
	case 1:
		return seeMenu("hamburger");
		break;
	case 2:
		return seeMenu("beverage");
		break;
	case 3:
		return seeMenu("dessert");
		break;
	case 4:
		return seeShopList();
		break;
	case 5:
		return orderMenu();
		break;
	default:
		printf("\n");
		printf("Terminate the ordering program.\n");
		mysql_close(conn);
		break;
	}
}

int seeMenu(char menuType[]) {
	snprintf(sql_query, sizeof(sql_query), "SELECT * FROM menus WHERE type = '%s'", menuType);
	mysql_query(conn, sql_query);
	sql_result = mysql_store_result(conn);

	menusNode* menusHead = NULL;

	printf("\n======[ %s Menu Table ]=======\n", menuType);
	int total = 0;
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		menusNode* newMenu = (menusNode*)malloc(sizeof(menusNode));
		strcpy(newMenu->name, sql_row[0]);
		newMenu->price = atoi(sql_row[1]);
		newMenu->next = NULL;

		if (menusHead == NULL)
			menusHead = newMenu;
		else {
			menusNode* cur = menusHead;
			while (cur->next != NULL) {
				cur = cur->next;
			}
			cur->next = newMenu;
		}

		printf("%d. %s (%swon)\n", total + 1, sql_row[0], commaValue(newMenu->price));
		total++;
	}

	mysql_free_result(sql_result);

	printf("0. Return to previous menu\n");
	printf("=====================================\n");
	printf("Select : ");
	int menuSelect = 0;
	scanf("%d", &menuSelect);
	while (1) {
		if (menuSelect == 0) {
			printf("Return to previous menu\n");
			return mainMenu();
			break;

		}else if(menuSelect <= total && menuSelect != 0){
			int i = 0;
			menusNode* cur = menusHead;
			while (cur != NULL) {
				if (++i == (menuSelect)) {
					return addMenuToList(cur->name, cur->price, menuType);
					break;
				}
				cur = cur->next;
			}
			break;

		}else {
			printf("\nYou made the wrong choice.\n");
			return seeMenu(menuType);
			break;
		}
	}

	menusNode* cur = menusHead;
	while (cur != NULL) {
		menusNode* nextCur = cur->next;
		free(cur);
		cur = nextCur;
	}
	menusHead = NULL;
}

int addMenuToList(char name[], int price, char menu[]) {
	int amount = 0;
	printf("\nHow many would you like to include %s? ", name);
	scanf("%d", &amount);
	price *= amount;

	shopListNode* newList = (shopListNode*)malloc(sizeof(shopListNode));
	newList->amount = amount;
	newList->price = price;
	strcpy(newList->name, name);
	newList->next = NULL;

	if (shopListHead == NULL)
		shopListHead = newList;
	else {
		shopListNode* cur = shopListHead;
		while (cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = newList;
	}

	printf("\nYou have added %d %s to your shopping cart\n",amount, name);
	return seeMenu(menu);
}

int seeShopList() {
	if (shopListHead != NULL) {
		int totalPrice = 0;
		int totalNum = 0;
		printf("\n========[ Shopping basket ]=========\n");

		shopListNode* cur = shopListHead;
		while (cur != NULL) {
			printf("%d. ( %s : %d ) %swon\n", ++totalNum, cur->name, cur->amount, commaValue(cur->price));
			totalPrice += cur->price;
			cur = cur->next;
		}

		printf("Sum : %swon\n", commaValue(totalPrice));
		printf("====================================\n");
		printf("\nEnter the number of the item you want to remove from the shopping cart: (0 is return) ");
		while (1) {
			int selectNum = 0;
			scanf("%d", &selectNum);
			if (selectNum <= totalNum && selectNum != 0) {
				int i = 0;
				shopListNode* cur = shopListHead;

				if (selectNum == 1) { // 1 is head Node
					if (shopListHead->next == NULL) {
						free(shopListHead);
						shopListHead = NULL;
					}
					else {
						shopListNode* prevHead = shopListHead;
						shopListHead = shopListHead->next;
						free(prevHead);
					}
				}
				else {
					while (cur != NULL) {
						if (++i == (selectNum - 1)) { // Previous node you want to delete
							shopListNode* deleteNode = (cur->next);
							cur->next = deleteNode->next;
							free(deleteNode);
						}
						cur = cur->next;
					}
				}

				return seeShopList();
				break;
			}
			else {
				printf("Return to previous menu\n");
				return mainMenu();
				break;
			}
		}
	}
	else {
		printf("\nThere are no items in your shopping cart\n");
		return mainMenu();
	}
}

int orderMenu() {
	if (shopListHead != NULL) {
		int totalPrice = 0;
		printf("\n=============[ Order ]==============\n");

		shopListNode* cur = shopListHead;
		while (cur != NULL) {
			printf("( %s : %d ) %swon\n", cur->name, cur->amount, commaValue(cur->price));
			totalPrice += cur->price;
			cur = cur->next;
		}

		printf("Sum : %swon\n", commaValue(totalPrice));
		printf("====================================\n");
		printf("\nHow much would you like to pay? (0 to cancel) ");
		int myMoney = 0;
		scanf("%d", &myMoney);
		if (myMoney > 0) {
			if (myMoney >= totalPrice) {
				shopListNode* cur = shopListHead;
				while (cur != NULL) {
					shopListNode* _cur = cur->next;
					free(cur);
					cur = _cur;
				}

				shopListHead = NULL;

				printf("\nYour order has been completed.\n");
				serialCommunication();
				printf("Left money: %swon\n", commaValue(myMoney - totalPrice));
				printf("\nType anything to return to the previous menu: ");
				return mainMenu();
			}
			else {
				printf("\n%swon is not enough.\n", commaValue(totalPrice - myMoney));
				return orderMenu();
			}
		}
		else {
			printf("\nYour order has been canceled.\n");
			return mainMenu();
		}

	} else {
		printf("\nFirst add the product to your shopping cart.\n");
		return mainMenu();
	}
};



