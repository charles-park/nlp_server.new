//-----------------------------------------------------------------------------
/**
 * @file main.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief 2025 nlp_server.new application
 * @version 0.1
 * @date 2025-01-10
 *
 * @copyright Copyright (c) 2022
 *
 */
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

//------------------------------------------------------------------------------
// for WiringPi
// apt install odroid-wiringpi libwiringpi-dev (wiringpi package)
//------------------------------------------------------------------------------
#include "lib_ioshield/lib_ioshield.h"
#include "lib_socket/lib_socket.h"
#include "lib_usblp/lib_usblp.h"

//------------------------------------------------------------------------------
pthread_t thread_nlp_server;
static int nlp_port_id = 0;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int get_board_ip (char *ip_str)
{
    FILE *fp;

    if ((fp = popen("hostname -I", "r")) != NULL) {
        memset (ip_str, 0x00, sizeof(struct sockaddr) +1);
        if (fgets(ip_str, sizeof(struct sockaddr), fp) != NULL) {
            printf ("%s : IP Address = %s\n", __func__, ip_str);
            pclose(fp);
            {
                char *tok;
                int i[4];

                tok = strtok(ip_str, ".");  i[0] = (tok != NULL) ? atoi(tok) : 0;
                tok = strtok(NULL  , ".");  i[1] = (tok != NULL) ? atoi(tok) : 0;
                tok = strtok(NULL  , ".");  i[2] = (tok != NULL) ? atoi(tok) : 0;
                tok = strtok(NULL  , ".");  i[3] = (tok != NULL) ? atoi(tok) : 0;

                memset (ip_str, 0x00, sizeof(struct sockaddr) +1);
                sprintf (ip_str, "%d.%d.%d.%d", i[0], i[1], i[2], i[3]);
            }
            return 1;
        }
        pclose(fp);
    }
err:
    memset  (ip_str, 0x00, sizeof(struct sockaddr) +1);
    strncpy (ip_str, "0.0.0.0", strlen("0.0.0.0") +1);
    return 0;
}

//------------------------------------------------------------------------------
static void time_display (int toffset)
{
    time_t t;
    char buf[64], len;

    time(&t);

    // time offset
    t += (toffset * 60 * 60);

    memset(buf, ' ', sizeof(buf));

    len = sprintf (buf, "Time %s", ctime(&t));
    buf [len -1] = ' ';
    ioshield_lcd_clear  (-1);
    ioshield_lcd_printf (0, 0, "%16s", &buf[0]);
    ioshield_lcd_printf (0, 1, "%16s", &buf[16]);
    fprintf(stdout, "%s : %s\n", __func__, buf);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int bt_callback (int bt_state)
{
    switch (bt_state) {
        case eBT1_PRESS:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT1_PRESS");
            break;
        case eBT1_RELEASE:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT1_RELEASE");
            nlp_port_id ++;
            if (!set_server_port (nlp_port_id)) nlp_port_id = 0;
            ioshield_led_byte (1 << nlp_port_id);
            break;
        case eBT2_PRESS:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT2_PRESS");
            break;
        case eBT2_RELEASE:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT2_RELEASE");
            nlp_port_id --;
            if (!set_server_port (nlp_port_id)) nlp_port_id = 0;
            ioshield_led_byte (1 << nlp_port_id);
            break;
        default :
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "Unknown");
            return 0;
    }
    return 1;
}

//------------------------------------------------------------------------------
static int socket_callback (char *msg, int r_size)
{
    printf ("%s : size = %d, msg = %s\n", __func__, r_size, msg);
    if (!strncmp(msg, "exit", strlen("exit")))
        return 0;

//    int usblp_print_mac (char *msg, int ch)
    usblp_print_mac ("00:1E:06:11:22:33", 0);

    return 1;
}

//------------------------------------------------------------------------------
void *thread_func_nlp_server(void *arg)
{
    char board_ip_str [sizeof(struct sockaddr)+1], onoff = 0;
    struct nlp_socket_info *s_info;

    while (1) {
        if (onoff) {
            get_board_ip (board_ip_str);
            ioshield_lcd_clear  (-1);
            ioshield_lcd_printf (0, 0, "%s", board_ip_str);
            s_info = get_server_port();
            ioshield_lcd_printf (0, 1, "%s(%d)", s_info->name, s_info->port);
        } else time_display (9);

        onoff = !onoff;
        sleep (2);
    }
    return arg;
}

//------------------------------------------------------------------------------
int main (int argc, char **argv)
{
    ioshield_init (bt_callback);

    ioshield_lcd_clear (-1);
    /* D1 ~ D7(1 ~ 7) */
    ioshield_led_byte (1 << nlp_port_id);

    socket_server_init (eBOARD_P_C4, socket_callback);

    usblp_config();

    pthread_create (&thread_nlp_server, NULL, thread_func_nlp_server, NULL);

    while (1)   {
        sleep(1);
    }

    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
