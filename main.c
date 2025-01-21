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
//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = toupper(*p);
}

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
        sprintf (ip_str, "%d.%d.%d.%d", 0, 0, 0, 0);
    }
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
static void usblp_status_display (void)
{
    char board_ip_str [sizeof(struct sockaddr)+1];
    struct nlp_socket_info *s_info;

    get_board_ip (board_ip_str);
    ioshield_lcd_clear  (-1);
    ioshield_lcd_printf (0, 0, "%s", board_ip_str);
    s_info = get_server_port();
    ioshield_lcd_printf (0, 1, "%s(%d)", s_info->name, s_info->port);
    ioshield_led_byte (1 << s_info->id);
}

//------------------------------------------------------------------------------
static void default_config_write (const char *fname);
const char *nlp_cfg_file = "/media/boot/nlp_port.cfg";
static int nlp_port_config = 0;

static int bt_callback (int bt_state)
{
    static int nlp_port_id = 0;
    struct nlp_socket_info *s_info;

    switch (bt_state) {
        case eBT1_PRESS:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT1_PRESS");
            break;
        case eBT1_LONG_PRESS:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT1_LONG_PRESS");
            nlp_port_config = 1;
            {
                s_info = get_server_port ();
                nlp_port_id = s_info->id;
            }
            break;
        case eBT1_RELEASE:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT1_RELEASE");
            if (nlp_port_config) {
                nlp_port_id ++;
                if (!set_server_port (nlp_port_id)) nlp_port_id = 0;
            }
            break;
        case eBT2_PRESS:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT2_PRESS");
            break;
        case eBT2_LONG_PRESS:
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT2_LONG_PRESS");
            // app restart
            if (nlp_port_config) {
                // /media/boot/nlp_config.txt update
                ioshield_lcd_clear  (-1);
                ioshield_lcd_printf (0, 0, "%s", "NLP PORT UPDATE");
                default_config_write (nlp_cfg_file);

                ioshield_lcd_printf (0, 1, "%s", "SYSTEM RESTART!");
                sleep (1);
                exit(0);
            }
            break;
        case eBT2_RELEASE:
            if (nlp_port_config) {
                if (nlp_port_id)    nlp_port_id --;
                if (!set_server_port (nlp_port_id)) nlp_port_id = 0;
            }
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "eBT2_RELEASE");
            break;
        default :
            printf ("%s : bt state = %d, %s\n", __func__, bt_state, "Unknown");
            return 0;
    }
    if (nlp_port_config) {
        s_info = get_server_port ();
        ioshield_lcd_clear  (-1);
        ioshield_lcd_printf (0, 0, "%s", "NLP PORT CONFIG");
        ioshield_lcd_printf (0, 1, "%s(%d)", s_info->name, s_info->port);
        ioshield_led_byte (1 << s_info->id);
    }
    return 1;
}

//------------------------------------------------------------------------------
static int print_mac (char *msg, int ch)
{
    char mac [20], *ptr;

    toupperstr (msg);
    if ((ptr = strstr (msg, "001E06")) != NULL) {
        memset  (mac, 0, sizeof(mac));
        sprintf (mac, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
            ptr[ 0], ptr[ 1], ptr[ 2], ptr[ 3], ptr[ 4], ptr[ 5],
            ptr[ 6], ptr[ 7], ptr[ 8], ptr[ 9], ptr[10], ptr[11]);

        usblp_print_mac (mac, ch);
    }
    return 1;
}

//------------------------------------------------------------------------------
#define NLP_ERR_LINE    3
#define NLP_ERR_CHAR    19

static int print_err (char *msg, int ch)
{
    char err_msg[NLP_ERR_LINE][NLP_ERR_CHAR], *ptr;
    int pos = 0, line = 0;

    if ((ptr = strtok (msg, ",")) != NULL)  {
        memset (err_msg, 0, sizeof(err_msg));
        while ((ptr = strtok (NULL, ",")) != NULL) {
            if ((pos + strlen (ptr)) >= NLP_ERR_CHAR) {
                pos = 0;
                if (++line >= NLP_ERR_LINE) {
                    usblp_print_err (&err_msg[0][0], &err_msg[1][0], &err_msg[2][0], ch);
                    memset (err_msg, 0, sizeof(err_msg));
                    line = 0;
                }
            }
            pos += sprintf (&err_msg[line][pos], "%s ", ptr);
        }
        usblp_print_err (&err_msg[0][0], &err_msg[1][0], &err_msg[2][0], ch);
    }
    return 1;
}

//------------------------------------------------------------------------------
pthread_t thread_iperf3;
static int ThreadIperf3 = 0, Iperf3Speed = 0;

static void *thread_iperf3_func (void *arg)
{
    FILE *fp;
    char cmd_line [1024], *pstr = NULL;

    printf ("%s : thread running!\n", __func__);
    ThreadIperf3 = 1;
    memset (cmd_line, 0, sizeof(cmd_line));
    if ((fp = popen("iperf3 -s -1", "r")) != NULL) {
        while (fgets(cmd_line, sizeof(cmd_line), fp)) {
            if (strstr (cmd_line, "receiver") != NULL) {
                if ((pstr = strstr (cmd_line, "MBytes")) != NULL) {
                    while (*pstr != ' ')    pstr++;
                    Iperf3Speed = atoi (pstr);
                    break;
                }
            }
            memset (cmd_line, 0, sizeof(cmd_line));
        }
        pclose(fp);
    }
    printf ("%s : thread stop! iperf3 speed = %d\n", __func__, Iperf3Speed);
    ThreadIperf3 = 0;
    return arg;
}

//------------------------------------------------------------------------------
const char *iperf3_stop_cmd = "ps ax | grep iperf3 | awk '{print $1}' | xargs kill";

static void thread_iperf3_stop (void)
{
    FILE *fp;
    char cmd_line [1024];

    memset (cmd_line, 0, sizeof(cmd_line));
    if ((fp = popen(iperf3_stop_cmd, "w")) != NULL) {
        pclose(fp);
    }
    ThreadIperf3 = 0;
}

//------------------------------------------------------------------------------
const char *resp_version = "20220419";

static int socket_callback (int c_fd, char *msg, int r_size)
{
    printf ("%s : size = %d, msg = %s\n", __func__, r_size, msg);
    if (!strncmp(msg, "exit", strlen("exit"))) {
        if (ThreadIperf3)   thread_iperf3_stop ();

        return 0;
    }

    if (strstr (msg, "mac")     != NULL)    print_mac (msg, 0);
    if (strstr (msg, "left-m")  != NULL)    print_mac (msg, 0);
    if (strstr (msg, "right-m") != NULL)    print_mac (msg, 1);

    if (strstr (msg, "error")   != NULL)    print_err (msg, 0);
    if (strstr (msg, "left-e")  != NULL)    print_err (msg, 0);
    if (strstr (msg, "right-e") != NULL)    print_err (msg, 1);

    if (strstr (msg, "iperf")   != NULL) {
        if ((!ThreadIperf3) && (strstr (msg, "start") != NULL))
            pthread_create (&thread_iperf3, NULL, thread_iperf3_func, NULL);

        if (( ThreadIperf3) && (strstr (msg, "stop")  != NULL))
            thread_iperf3_stop ();
    }
    if (strstr (msg, "version") != NULL)
        send (c_fd, resp_version, strlen(resp_version), 0);

    return 0;
}

//------------------------------------------------------------------------------
pthread_t thread_nlp_server;

void *thread_func_nlp_server(void *arg)
{
    char onoff = 0;

    while (1) {
        if (!nlp_port_config) {
            if (onoff)  usblp_status_display ();
            else        time_display (9);
            onoff = !onoff;
        }
        sleep (2);
    }
    return arg;
}

//------------------------------------------------------------------------------
static void default_config_write (const char *fname)
{
    FILE *fp;
    char value [1024];
    struct nlp_socket_info *s_info;

    if ((fp = fopen(fname, "wt")) == NULL)
        return;

    // default value write
    fputs   ("# info : id, port, name \n", fp);
    memset  (value, 0, sizeof(value));

    s_info = get_server_port ();
    sprintf (value, "%d,%d,%s,\n", s_info->id, s_info->port, s_info->name);
    fputs   (value, fp);
    fclose  (fp);
}

//------------------------------------------------------------------------------
static void default_config_read (void)
{
    FILE *fp;
    char value [1024], *ptr;

    if (access (nlp_cfg_file, R_OK) != 0) {
        // default value (8888)
        socket_server_init (eBOARD_P_C4, socket_callback);

        default_config_write (nlp_cfg_file);
        return;
    }

    if ((fp = fopen(nlp_cfg_file, "r")) == NULL)
        return;

    while(1) {
        memset (value , 0, sizeof(value));
        if (fgets (value, sizeof (value), fp) == NULL)
            break;

        switch (value[0]) {
            case '#':   case '\n':
                break;
            default :
                // id, port, name
                {
                    struct nlp_socket_info *s_info;

                    if ((ptr = strtok (value, ",")) != NULL) {
                        socket_server_init (atoi(ptr), socket_callback);;
                        s_info = get_server_port ();
                        printf ("%s : id = %d, port = %d, name = %s\n",
                                __func__, s_info->id, s_info->port, s_info->name);
                    }
                }
                break;
        }
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
int main (void)
{
    ioshield_init (bt_callback);

    ioshield_lcd_clear (-1);
    /* D1 ~ D7(1 ~ 7) */
    ioshield_led_byte (0);

    ioshield_lcd_printf (0, 0, "%s", " USB LP-SERVER  ");
    ioshield_lcd_printf (0, 1, "%s", "  Initialize... ");
    usblp_config();

    // /media/nlp_config.txt read
    default_config_read ();

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
