#include <stdlib.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/api.h"
#include "lwip/sys.h"

#if 0


#define TCP_SERVER_PORT  1234


/****************************************************************************************
raw_api
****************************************************************************************/
#if 0
err_t tcp_client_connect(const unsigned char *destip, unsigned short port)
{
	struct tcp_pcb *client_pcb;
	struct ip_addr DestIPaddr;
	err_t err = ERR_OK;

	/* create new tcp pcb */
	client_pcb = tcp_new();
	client_pcb->so_options |= SOF_KEEPALIVE;

#if 1
	client_pcb->keep_idle = 50000;	   // ms
	client_pcb->keep_intvl = 20000;	   // ms
	client_pcb->keep_cnt = 5;
#endif	

	if (client_pcb != NULL)
	{
		IP4_ADDR(&DestIPaddr, *destip, *(destip + 1), *(destip + 2), *(destip + 3));

		/* connect to destination address/port */
		err = tcp_connect(client_pcb, &DestIPaddr, port, tcp_client_connected);
		tcp_err(client_pcb, tcp_errf);
	}

	return err;
}


static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{

	if (es == NULL)
	{
		es = (struct client *)mem_malloc(sizeof(struct client));
	}

	switch (err) 
	{
		case ERR_OK:
			es->pcb = tpcb;                                  /*?????????????*/
			es->p_tx = NULL;
			es->state = ES_CONNECTED;
			tcp_arg(tpcb, es);                       /*????????????????   */
			tcp_recv(tpcb, tcp_client_recv);

			break;
		case ERR_MEM:
			tcp_client_connection_close(tpcb, es);
			break;
		default:
			break;
	}

	return err;
}


static void tcp_errf(void *arg, err_t err) 
{
	printf("\r\ntcp_errf be called...\r\n");
	
	if (es == NULL)
	{
		es = (struct client *)mem_malloc(sizeof(struct client));
		es = (struct client *)arg;
	}

	if (err == ERR_OK) 
	{
		/* No error, everything OK. */
		return;
	}

	switch (err)
	{
	case ERR_MEM:                                            /* Out of memory error.     */
		printf("\r\n ERR_MEM   \r\n");
		break;
	case ERR_BUF:                                            /* Buffer error.            */
		printf("\r\n ERR_BUF   \r\n");
		break;
	case  ERR_TIMEOUT:                                       /* Timeout.                 */
		printf("\r\n ERR_TIMEOUT   \r\n");
		break;
	case ERR_RTE:                                            /* Routing problem.         */
		printf("\r\n ERR_RTE   \r\n");
		break;
	case ERR_ISCONN:                                          /* Already connected.       */
		printf("\r\n ERR_ISCONN   \r\n");
		break;
	case ERR_ABRT:                                           /* Connection aborted.      */
		printf("\r\n ERR_ABRT   \r\n");
		break;
	case ERR_RST:                                            /* Connection reset.        */
		printf("\r\n ERR_RST   \r\n");
		break;
	case ERR_CONN:                                           /* Not connected.           */
		printf("\r\n ERR_CONN   \r\n");
		break;
	case ERR_CLSD:                                           /* Connection closed.       */
		printf("\r\n ERR_CLSD   \r\n");
		break;
	case ERR_VAL:                                            /* Illegal value.           */
		printf("\r\n ERR_VAL   \r\n");
		return;
	case ERR_ARG:                                            /* Illegal argument.        */
		printf("\r\n ERR_ARG   \r\n");
		return;
	case ERR_USE:                                            /* Address in use.          */
		printf("\r\n ERR_USE   \r\n");
		return;
	case ERR_IF:                                             /* Low-level netif error    */
		printf("\r\n ERR_IF   \r\n");
		break;
	case ERR_INPROGRESS:                                     /* Operation in progress    */
		printf("\r\n ERR_INPROGRESS   \r\n");
		break;

	}

	es->state = ES_CLOSING;
	err_process();
}


/**
* @brief tcp_receiv callback
* @param arg: argument to be passed to receive callback
* @param tpcb: tcp connection control block
* @param err: receive error code
* @retval err_t: retuned error
*/
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct client *es;
	err_t ret_err;
	LWIP_ASSERT("arg != NULL", arg != NULL);
	es = (struct client *)arg;

	if (p == NULL)
	{
		/* remote host closed connection */
		es->state = ES_CLOSING;
		if (es->p_tx == NULL)
		{
			/* we're done sending, close connection */
			tcp_client_connection_close(tpcb, es);
		}
		ret_err = ERR_OK;
	}
	else if (err != ERR_OK)
	{
		/* free received pbuf*/
		if (p != NULL)
		{
			pbuf_free(p);
		}
		ret_err = err;
	}
	else if (es->state == ES_CONNECTED)
	{
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);                   //??p?????

		memset(recevRxBufferTcpEth, 0x00, TCPREVDATALEN);

		if (p->len > TCPREVDATALEN)
		{
			p->len = TCPREVDATALEN;
		}
		memcpy(&recevRxBufferTcpEth, p->payload, p->len);
		tcpRecevieFlag = 1;                    //?tcp???1.?????????????
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	/* data received when connection already closed */
	else
	{
		/* Acknowledge data reception */
		tcp_recved(tpcb, p->tot_len);

		/* free pbuf and do nothing */
		pbuf_free(p);
		ret_err = ERR_OK;
	}

	return ret_err;
}


/**
* @brief function used to send data
* @param  tpcb: tcp control block
* @param  es: pointer on structure of type client containing info on data
*             to be sent
* @retval None
*/
void tcp_client_send(struct tcp_pcb *tpcb, struct client * es)
{
	struct pbuf *ptr;
	err_t wr_err = ERR_OK;
	while ((wr_err == ERR_OK) &&
		(es->p_tx != NULL) &&
		(es->p_tx->len <= tcp_sndbuf(tpcb)))
	{
		/* get pointer on pbuf from es structure */
		ptr = es->p_tx;

		wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

		if (wr_err == ERR_OK)
		{
			/* continue with next pbuf in chain (if any) */
			es->p_tx = ptr->next;

			if (es->p_tx != NULL)
			{
				/* increment reference count for es->p */
				pbuf_ref(es->p_tx);
			}
			pbuf_free(ptr);
		}
		else if (wr_err == ERR_MEM)
		{
			/* we are low on memory, try later, defer to poll */
			es->p_tx = ptr;
		}
		else
		{
			es->p_tx = ptr;
			/* other problem ?? */
		}
	}
}


//user send message 
u8_t tcp_send_message(void *msg, uint16_t len) {
	u8_t  count = 0;
	struct pbuf *p;
	if (es->state != ES_CONNECTED)  return -1;
	if (es->p_tx == NULL) {

		es->p_tx = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
		pbuf_take(es->p_tx, (char*)msg, len);
	}
	tcp_client_send(es->pcb, es);
	return 1;
}




/**
* @brief This function is used to close the tcp connection with server
* @param tpcb: tcp connection control block
* @param es: pointer on client structure
* @retval None
*/
static void tcp_err_close()
{
	/* remove callbacks */
	tcp_recv(es->pcb, NULL);

	if (es != NULL)
	{
		if (es->p_tx != NULL) {
			pbuf_free(es->p_tx);
		}
		if (es->pcb != NULL) {
			tcp_close(es->pcb);
		}
		mem_free(es);
	}
}


static void tcp_client_connection_close()
{
	/* remove callbacks */
	tcp_recv(es->pcb, NULL);
	if (es != NULL)
	{
		if (es->p_tx != NULL) {
			pbuf_free(es->p_tx);
		}
		if (es->pcb != NULL) {
			/* close tcp connection */
			tcp_close(es->pcb);
		}
		mem_free(es);
		es = NULL;
	}
	set_timer4_countTime(TIMER_5000MS);
}

//????????tcp????????????????
static void err_process() {
	connet_flag++;
	if (connet_flag <= 3) {
		set_timer4_countTime(TIMER_5000MS);
	}
	if (connet_flag > 3) {
		connet_flag = 4;
		set_timer4_countTime(TIMER_10000MS);
	}
}
//?????????????????
void reconnet_tcp_timer() 
{
	if (es != NULL) 
	{
		tcp_err_close(es->pcb, es);
		tcp_client_connect(ip_des, destcp_port);
	}
	else
	{
		tcp_client_connect(ip_des, destcp_port);
	}
}
//?????????,????????
int W301ProcessRecvTcpData() 
{

}
#endif

/****************************************************************************************
netconn_api
****************************************************************************************/

void TcpServerNetConn(void *arg)
{
	struct netconn *conn, *newconn;
	err_t err;

	LWIP_UNUSED_ARG(arg);

	printf("TcpServerNetConn...\n");
	/* Create a new connection identifier. */
	conn = netconn_new(NETCONN_TCP);

	if (conn != NULL)
	{
		err = netconn_bind(conn, NULL, TCP_SERVER_PORT);

		if (err == ERR_OK)
		{
			/* Tell connection to go into listening mode. */
			netconn_listen(conn);

			printf("Listening...\n");

			while (1)
			{
				/* Grab new connection. */
				err = netconn_accept(conn, &newconn);
				printf("accept, err:%d\n", err);

				/* Process the new connection. */
				if (err == ERR_OK)
				{
					struct netbuf *buf;
					void *data;
					u16_t len;

					while (1)
					{
						err = netconn_recv(newconn, &buf);
						printf("netconn_recv:%d\n", err);

						if (buf != NULL)
						{
							do
							{
								netbuf_data(buf, &data, &len);
								netconn_write(newconn, data, len, NETCONN_COPY);
							} while (netbuf_next(buf) >= 0);

							netbuf_delete(buf);
						}
					}

					/* Close connection and discard connection identifier. */
					printf("TcpConnect Close newconn\n");
					netconn_close(newconn);
					netconn_delete(newconn);
				}
			}
		}
		else
		{
			printf(" can not bind TCP netconn");
		}
	}
	else
	{
		printf("can not create TCP netconn");
	}
}

/****************************************************************************************
socket_api
****************************************************************************************/

void TcpServerSocket(void *pvParameters)
{
	long lSocket, lClientFd, lBytes, lAddrLen = sizeof(struct sockaddr_in);
	struct sockaddr_in sLocalAddr;
	struct sockaddr_in client_addr;
	signed char Data[32];

	(void)pvParameters;

	printf("TcpServerSocket...\n");

	lSocket = lwip_socket(AF_INET, SOCK_STREAM, 0);

	if (lSocket < 0)
	{
		vTaskDelete(NULL);
		return;
	}

	memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
	sLocalAddr.sin_family = AF_INET;
	sLocalAddr.sin_len = sizeof(sLocalAddr);
	sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sLocalAddr.sin_port = ntohs(((unsigned short)TCP_SERVER_PORT));

	if (lwip_bind(lSocket, (struct sockaddr *) &sLocalAddr, sizeof(sLocalAddr)) < 0)
	{
		lwip_close(lSocket);
		vTaskDelete(NULL);
	}

	if (lwip_listen(lSocket, 20) != 0)
	{
		lwip_close(lSocket);
		vTaskDelete(NULL);
	}

	printf("Listening...\n");

	for ( ;; )
	{
		lClientFd = lwip_accept(lSocket, (struct sockaddr *) &client_addr, (u32_t *)&lAddrLen);
		printf("accept, lClientFd:%d\n", lClientFd);

		if (lClientFd > 0L)
		{
			do
			{
				lBytes = lwip_recv(lClientFd, Data, sizeof(Data), 0);

				if (lBytes > 0L)
				{
					lwip_send(lClientFd, Data, lBytes, 0);
				}

			} while (lBytes > 0L);

			printf("TcpConnect Close lClientFd\n");
			lwip_close(lClientFd);
		}
	}/*for (;; )*/


	 /* Will only get here if a listening socket could not be created. */
	vTaskDelete(NULL);
}


#endif 