#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"


#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "nvs_flash.h"

#include "esp_spiffs.h"

#include "esp_vfs.h"
#include "spiffs_vfs.h"


// global variables
char actual_path[256];

// Event group
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
//#define CONFIG_RESOURCE2 "/news/"
//#define CONFIG_WEBSITE2 "conline.890m.com"
#define CONFIG_RESOURCE2 "/"
#define CONFIG_WEBSITE2 "192.168.1.103"
static const char *serveraddr = CONFIG_WEBSITE2;

// HTTP request
static const char *REQUEST = "GET "CONFIG_RESOURCE2" HTTP/1.1\r\n"
	"Host: "CONFIG_WEBSITE2" \r\n"
	"User-Agent: ESP32\r\n"
	"\r\n";

// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
		
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    
	case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		//printf("received event SYSTEM_EVENT_STA_GOT_IP %d", SYSTEM_EVENT_STA_GOT_IP);
        break;
    
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    
	default:
        break;
    }
   
	return ESP_OK;
}


// Main task
void main_task(void *pvParameter)
{
	// wait for connection
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	printf("connected!\n");
	printf("\n");
	
	// print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	//printf("IP Address: ");
	//printf(IPSTR, &ip_info.ip);
	printf("\n\n");
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	printf("\n");
	
	/* define connection parameters
	const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };*/
	
	// address info struct and receive buffer
    //struct addrinfo *res;
	char recv_buf[100];
	
	// resolve the IP of the target website
	/*int result = getaddrinfo(CONFIG_WEBSITE2, "80", &hints, &res);

	if((result != 0) || (res == NULL)) {
		printf("Unable to resolve IP for target website %s\n", CONFIG_WEBSITE2);
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	printf("Target website's IP resolved\n");
	printf("res->ai_protocol %d\n", res->ai_protocol);
	printf("res->ai_socktype %d\n", res->ai_socktype);
	printf("res->ai_addrlen %d\n", res->ai_addrlen);
	printf("res->ai_family %d\n", res->ai_family);

	printf("s->ai_addr->sa_len %d:\n", res->ai_addr->sa_len);*/

	/*static const char *serveraddr = CONFIG_WEBSITE2;
	struct sockaddr aiaddr;
	ipaddr_aton(serveraddr, &aiaddr);
	struct addrinfo risultato = {
	        .ai_family = AF_INET,
	        .ai_socktype = SOCK_STREAM,
			.ai_addrlen = 28,
	        .ai_addr = &aiaddr,
	    };

	res = &risultato;*/
	// create a new socket
	//int s = socket(res->ai_family, res->ai_socktype, 0);

	int s = socket(AF_INET,SOCK_STREAM,0);
	if(s < 0) {
		printf("Unable to allocate a new socket\n");
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	} else {
		printf("socket %d, \n",s);
		printf("domain %d, \n",AF_INET);
		printf("type %d, \n",SOCK_STREAM);
		printf("Socket allocated, id=%d\n", s);
	}
	
	// connect to the specified server
	//result = connect(s, res->ai_addr, res->ai_addrlen);

	struct sockaddr_in serAddr;
	memset((void *) &serAddr, 0, sizeof(serAddr)); /* clear server address */
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(80);
	//serAddr.sin_len = 28;
	/* build address using inet_pton */
	if ( (inet_pton(AF_INET, serveraddr, &serAddr.sin_addr)) <= 0) {
	         perror("Address creation error");
	         while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	int result = connect(s, (struct sockaddr*) &serAddr , sizeof(serAddr));

	if(result < 0) {
		printf("Unable to connect to the target website\n");
		printf("serAddr.sin_addr.s_addr %d, \n",serAddr.sin_addr.s_addr);
		printf("serAddr.sin_len %d, \n",serAddr.sin_len);
		printf("errcode %d, \n",result);
		close(s);
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	} else {
		printf("Connected to the target website: %s\n", serveraddr);
		printf("serAddr.sin_addr.s_addr %d, \n",serAddr.sin_addr.s_addr);
		printf("serAddr.sin_len %d, \n",serAddr.sin_len);
		vTaskDelay(6000 / portTICK_RATE_MS);
	}
	
	// send the request
	ESP_LOGI("my_example:", "Richiesta Inviata ESP_LOGI: %s\n\n\n",REQUEST);
	//ESP_LOGI(TAG, "Boot count: %d", boot_count);
	printf("Richiesta inviata %s\n\n\n",REQUEST);
	result = write(s, REQUEST, strlen(REQUEST));
		if(result < 0) {
		printf("Unable to send the HTTP request\n");
		close(s);
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	printf("HTTP request sent\n");
	char file_path_out[300];
	sprintf(file_path_out, "/spiffs/received.out");
	printf("Saving content to file %s\r\n", file_path_out);
	
	// print the response
	printf("HTTP response:\n");
	printf("--------------------------------------------------------------------------------\n");
	int r,total;
	total=0;
	
	FILE *file_out;
	file_out = fopen(file_path_out, "w");
		
	if (!file_out) {
        printf("Error opening file output %s\r\n", file_path_out);
        return;
    }
	
	do {
		bzero(recv_buf, sizeof(recv_buf));
		r = read(s, recv_buf, sizeof(recv_buf) - 1);
		total+=r;
		for(int i = 0; i < r; i++) {
			putchar(recv_buf[i]);
			fputc(recv_buf[i],file_out);
		}
	} while(r > 0);	
	printf("--------------------------------------------------------------------------------\n");
	close(s);
	// close the folder
	fclose(file_out);
	printf("Socket closed\n");
	printf("total received characters %d\n",total);
	//printf("Content of file %s\r\n", file_path_out);
	
	int i = 0;
	while(i) {
		// display the saved file content
		int filechar;
		FILE *file_in;
		file_in = fopen(file_path_out, "r");
		printf("--------------------------------------------------------------------------------\n");
		printf("\r\n\r\n\r\nPrinting  content of file %s\r\n", file_path_out);
		printf("--------------------------------------------------------------------------------\n");
		while((filechar = fgetc(file_in)) != EOF){
			putchar(filechar);
		}
		fclose(file_in);
		vTaskDelay(10000 / portTICK_RATE_MS);
	}
	while(1) vTaskDelay(10000 / portTICK_RATE_MS);

}


// Main application
void app_main()
{
	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);
	
	// create the event group to handle wifi events
	wifi_event_group = xEventGroupCreate();
		
	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	
	// initialize the wifi stack in STAtion mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	// configure the wifi connection and start the interface
	wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    	ESP_ERROR_CHECK(esp_wifi_start());
	printf("Connecting to %s... ", CONFIG_WIFI_SSID);
	
	printf("SPIFFS example\r\n\r\n");
	
	// register SPIFFS with VFS
	vfs_spiffs_register();
	
	// the partition was mounted?
	if(spiffs_is_mounted) {
		
		printf("Partition correctly mounted!\r\n");
	} 
	
	else {
		printf("Error while mounting the SPIFFS partition");
		while(1) vTaskDelay(1000 / portTICK_RATE_MS);
	}
	
	// initial path
	strcpy(actual_path, "/spiffs");

	// start the main task
    xTaskCreate(&main_task, "main_task", 8192, NULL, 5, NULL);
}
