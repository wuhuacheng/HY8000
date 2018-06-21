/*****************************************************************************


*****************************************************************************/

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/api.h"
#include "lwip/sys.h"

#include "stm32f4xx.h"

#define  UDP_SRV_PORT	12128
#define  RAMLOAD_BASE				0x20000000
#define  FLSLOAD_BASE			    0x08020000

#define  HANDSHAKE_CMD			 	0xA001

#define  WRITE_CMD					0xA101
#define  READ_CMD					0xA102
#define  BLKWRITE_CMD				0xA103
#define  BLKREAD_CMD				0xA104

#define  CMD_RESPONSE_OK			0xACAC
#define  CMD_RESPONSE_ERR			0xABAD
#define  HANDSHAKE_RESPONSE_OK		CMD_RESPONSE_OK

void writeFlash(uint32_t addr,  uint32_t *pData, uint32_t len);
void ToggleLed(void);

/*set Main Stack value*/
__asm void __MSR_MSP(unsigned int addr) 
{
     MSR MSP, r0
     BX  r14
}


typedef  void (*pFunction)(void);
uint32_t SimRegs[8];
uint32_t rxBuff32[16];
unsigned short feedBack[16];

void UdpLoadServer(void const *argument)
{
    unsigned short cmd = 0;
    unsigned short cmdLen = 0;
    unsigned short regIndex = 0;
    unsigned int blkwrPtr = 0;
    unsigned int blkwrTotalLen = 0;
	unsigned int fileChkSum = 0;
	unsigned int checkSum = 0;
    
    int UdpSocket = 0;
	struct sockaddr_in sLocalAddr;
	struct sockaddr_in clientAddr;
	int len = sizeof(clientAddr);
    uint8_t *rxBuff8 = (uint8_t *)(rxBuff32);
    uint16_t *rxBuff16 = (uint16_t *)(rxBuff32);

    int n;
    int i = 0;    
 
   
    unsigned char *Data = (unsigned char*)(RAMLOAD_BASE);
    pFunction  ramNewApp = (pFunction)(RAMLOAD_BASE+0x00000004);

    (void)argument;
 
	UdpSocket = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (UdpSocket < 0)
	{
		vTaskDelete(NULL);
		return;
	}

	sLocalAddr.sin_family = AF_INET;
	sLocalAddr.sin_len = sizeof(sLocalAddr);
	sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sLocalAddr.sin_port = ntohs(((unsigned short)UDP_SRV_PORT));
	
	if (lwip_bind(UdpSocket, (struct sockaddr *) &sLocalAddr, sizeof(sLocalAddr)) < 0)
	{
		lwip_close(UdpSocket);
		vTaskDelete(NULL);		
		return;
	}

	while (1)
	{
        n = lwip_recvfrom(UdpSocket, rxBuff8, 64, 0, (struct sockaddr*)&clientAddr, (socklen_t*)(&len));

		if (n <= 0)
		{
            break;
        }
              
        cmd = rxBuff16[0];
		cmdLen = rxBuff16[1]; 		
		regIndex = rxBuff16[1];
		
		switch (cmd)
		{
			case HANDSHAKE_CMD:
				feedBack[0] = HANDSHAKE_RESPONSE_OK;
				len = 2;
				break;
				
			case WRITE_CMD:
				if (regIndex < 16)
				{
					SimRegs[regIndex] = rxBuff32[1];                    
					feedBack[0] = HANDSHAKE_RESPONSE_OK; 
				}
				else
				{
					feedBack[0] = CMD_RESPONSE_ERR;
				}
				len = 2;
				break;

			case READ_CMD:
				if (regIndex < 16)
				{
					feedBack[0] = CMD_RESPONSE_OK;
					feedBack[1] = (SimRegs[regIndex] & 0xFFFF);
					feedBack[2] = ((SimRegs[regIndex] >> 16) & 0xFFFF);
				}
				else
				{
					feedBack[0] = CMD_RESPONSE_ERR;
					feedBack[1] = 0;
					feedBack[2] = 0;
				}

				len = 6;
				break;

			case BLKWRITE_CMD:
				feedBack[0] = CMD_RESPONSE_OK;
                blkwrPtr = rxBuff32[1];
                blkwrTotalLen = rxBuff32[2];
                
                for (i=0; i<(cmdLen-8); i++)
                {
                    Data[blkwrPtr+i] = rxBuff8[12+i];
                }
                                
				len = 2;
				break;

			case BLKREAD_CMD:
				feedBack[0] = CMD_RESPONSE_OK;
         
				len = 20;
				break;

			default:
				feedBack[0] = CMD_RESPONSE_ERR;
				len = 2;
				break;
		}/*switch*/
						
		n = lwip_sendto(UdpSocket, feedBack, len, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

        if (n < 0)
        {
            break;
        }
        
        if ((cmd==BLKWRITE_CMD) && (blkwrTotalLen == (blkwrPtr + cmdLen - 8)))
        {
            fileChkSum = SimRegs[0];
			checkSum = 0;	
			for (i = 0; i <blkwrTotalLen; i++)
			{
				checkSum += (unsigned char)Data[i];
			}
            
            if (checkSum == fileChkSum)
			{
                __disable_irq();  
			    ramNewApp = (pFunction)(*(volatile unsigned int*)(RAMLOAD_BASE+4));
                SCB->VTOR = RAMLOAD_BASE;
                __MSR_MSP(*(volatile unsigned int*)(RAMLOAD_BASE));
                ramNewApp();
			}					
        }
        
	} /*while(1)*/
	
	lwip_close(UdpSocket);
	vTaskDelete(NULL);
}



void JumpToNewRamApp(uint32_t AppAddr, uint32_t TotalLen, uint32_t fileChkSum)
{
    uint32_t checkSum = 0;	
    uint32_t i = 0;
    unsigned char *Data = (unsigned char*)(RAMLOAD_BASE);
    pFunction  ramNewApp = (pFunction)(RAMLOAD_BASE+0x00000004);
    
    for (i = 0; i <TotalLen; i++)
    {
        checkSum += (unsigned char)Data[i];
    }
    
    if (checkSum == fileChkSum)
    {
        __disable_irq();  
        ramNewApp = (pFunction)(*(volatile unsigned int*)(RAMLOAD_BASE+4));
        SCB->VTOR = RAMLOAD_BASE;
        __MSR_MSP(*(volatile unsigned int*)(RAMLOAD_BASE));
        ramNewApp();
    }			
}

void JumpToFlsApp(uint32_t DataAddr, uint32_t FlsAddr, uint32_t TotalLen, uint32_t fileChkSum)
{
    uint32_t checkSum = 0;	
    uint32_t i = 0;
    unsigned char *Data = (unsigned char*)(RAMLOAD_BASE);
    pFunction  ramNewApp = (pFunction)(RAMLOAD_BASE+0x00000004);
    
    for (i = 0; i <TotalLen; i++)
    {
        checkSum += (unsigned char)Data[i];
    }
    
    if (checkSum == fileChkSum)
    {
        __disable_irq();  
        ramNewApp = (pFunction)(*(volatile unsigned int*)(RAMLOAD_BASE+4));
        SCB->VTOR = RAMLOAD_BASE;
        __MSR_MSP(*(volatile unsigned int*)(RAMLOAD_BASE));
        ramNewApp();
    }	
}


#if 0

static void UdpLoad_Thread(void *arg)
{
    err_t err;
    int i = 0;
    struct netconn *conn;
    struct netbuf *buf;
    struct netbuf *bkbuf;
    struct ip_addr *addr;
    unsigned short port = 0;	
    unsigned short Cmd = 0;
    unsigned short Len = 0;
    unsigned short regIndex = 0;
    unsigned int Ptr = 0;
    unsigned int TotalLen = 0;
	unsigned int fileChkSum = 0;
	unsigned int checkSum = 0;
    unsigned short feedBack[3];    
    unsigned char *Data = (unsigned char*)(RAMLOAD_BASE);
    pFunction  NewApp = (pFunction)(RAMLOAD_BASE+0x00000004);
    LWIP_UNUSED_ARG(arg);
    
    conn = netconn_new(NETCONN_UDP);

    if (conn == NULL)
    {
        return;
    }
	
    err = netconn_bind(conn, IP_ADDR_ANY, LOCAL_PORT);

    if (err != ERR_OK) 
    {
        return;
    }
	
    bkbuf = netbuf_new();	
    
    while (1)
    {
        buf = netconn_recv(conn);
        
        if (buf == NULL) 
        {
            continue;
        }
        
        addr = netbuf_fromaddr(buf);
        port = netbuf_fromport(buf);
                       
        netbuf_copy_partial(buf, &Cmd, 2, 0);
        netbuf_copy_partial(buf, &Len, 2, 2);
        regIndex = Len;
        
        switch (Cmd)
        {
            case HANDSHAKE_CMD:
                feedBack[0] = HANDSHAKE_RESPONSE_OK;
                netbuf_alloc(bkbuf, 2);
                netbuf_take(bkbuf, feedBack, 2);
                break;
            
            case WRITE_CMD:
                if (regIndex < 8)
				{
					netbuf_copy_partial(buf, &(SimRegs[regIndex]), 4, 4);
					feedBack[0] = HANDSHAKE_RESPONSE_OK;
				}
				else
				{
					feedBack[0] = CMD_RESPONSE_ERR;
				}
                netbuf_alloc(bkbuf, 2);
                netbuf_take(bkbuf, feedBack, 2);
                break;
            
            case READ_CMD:
                if (regIndex < 8)
				{
					feedBack[0] = CMD_RESPONSE_OK;
					feedBack[1] = (SimRegs[regIndex] & 0xFFFF);
					feedBack[2] = ((SimRegs[regIndex] >> 16) & 0xFFFF);
				}
				else
				{
					feedBack[0] = CMD_RESPONSE_ERR;
					feedBack[1] = 0;
					feedBack[2] = 0;
				}
                netbuf_alloc(bkbuf, 2+4);
                netbuf_take(bkbuf, feedBack, 2+4);
                break;           

            case BLKWRITE_CMD:
                netbuf_copy_partial(buf, &Ptr, 4, 4);
                netbuf_copy_partial(buf, &TotalLen, 4, 8);
                netbuf_copy_partial(buf, &Data[Ptr], Len - 8, 12);

                feedBack[0] = CMD_RESPONSE_OK;
                netbuf_alloc(bkbuf, 2);
                netbuf_take(bkbuf, feedBack, 2);
                break;

            case BLKREAD_CMD:/*NOT READY*/
                feedBack[0] = CMD_RESPONSE_OK;
                netbuf_alloc(bkbuf, 2);
                netbuf_take(bkbuf, feedBack, 2);
                break;

            default:
                feedBack[0] = CMD_RESPONSE_ERR;
                netbuf_alloc(bkbuf, 2);
                netbuf_take(bkbuf, feedBack, 2);
                break;
        }/*switch()*/
                    
        netbuf_delete(buf);				
        netconn_sendto(conn, bkbuf, addr, port);
        netbuf_free(bkbuf);
        
        /*Jump to New */
        if ((Cmd==BLKWRITE_CMD) && (TotalLen == (Ptr + Len - 8)))
        {
            fileChkSum = SimRegs[0];
			checkSum = 0;	
			for (i = 0; i <TotalLen; i++)
			{
				checkSum += (unsigned char)Data[i];
			}
            
            if (checkSum == fileChkSum)
			{
                __disable_irq();  
			    NewApp = (pFunction)(*(volatile unsigned int*)(RAMLOAD_BASE+4));
                SCB->VTOR = RAMLOAD_BASE;
                __MSR_MSP(*(volatile unsigned int*)(RAMLOAD_BASE));
                NewApp();
			}					
        }
        
    }/*while(1)*/
}
#endif



