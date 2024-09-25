#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>

int get_ip(char *hostname, char *ips);
int get_hostname(char *hostname, char *ip);
void print_alternate_names(struct hostent *host_info);
bool is_valid_ip_format(char *ip);
bool is_valid_domain_name(char *domain);
int check_option(int option, char *param);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <option: 1 or 2> <parameter: domain name or IP>\n", argv[0]);
        return 1;
    }

    int option = atoi(argv[1]);
    char *param = argv[2];

    if (check_option(option, param)) {
        if (option == 1) { // Option 1: IP to domain name
            char hostname[100];
            if (get_hostname(hostname, param) == 0) {
                printf("Main domain name: %s\n", hostname);
                char ips[256];
                if (get_ip(hostname, ips) == 0) {
                    printf("Related IP addresses: %s\n", ips);
                } else {
                    printf("No related IP addresses found for: %s\n", hostname);
                }
                struct hostent *host_info = gethostbyname(hostname);
                if (host_info) {
                    print_alternate_names(host_info);
                }
            } else {
                printf("No information found\n");
            }
        } else if (option == 2) { // Option 2: domain name to IP
            char ips[256];
            if (get_ip(param, ips) == 0) {
                printf("Official IP: %s\n", strtok(ips, " ")); // Main IP
                printf("Alias IP:\n");
                char *ip = strtok(NULL, " ");
                while (ip != NULL) {
                    printf("%s\n", ip);
                    ip = strtok(NULL, " ");
                }
            } else {
                printf("No information found\n");
            }
        } else {
            printf("Invalid option\n");
        }
    } else {
        printf("Invalid option\n");
    }

    return 0;
}

int get_ip(char *hostname, char *ips) {
    struct addrinfo hints, *res;
    int status;
    ips[0] = '\0'; 
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        return 1; // Not found
    }

    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        char ip[INET6_ADDRSTRLEN];
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(p->ai_family, &(ipv4->sin_addr), ip, sizeof(ip));
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            inet_ntop(p->ai_family, &(ipv6->sin6_addr), ip, sizeof(ip));
        }
        strcat(ips, ip);
        strcat(ips, " ");
    }

    freeaddrinfo(res); // Free the linked list
    return 0;
}

int get_hostname(char *hostname, char *ip) {
    struct in_addr addr;
    inet_aton(ip, &addr); 
    struct hostent *es = gethostbyaddr(&addr, sizeof(addr), AF_INET);

    if (es == NULL) {
        return 1; // Not found
    }
    strcpy(hostname, es->h_name);
    return 0;
}

void print_alternate_names(struct hostent *host_info) {
    if (host_info->h_aliases[0] != NULL) {
        printf("Alternate names:\n");
        for (char **alias = host_info->h_aliases; *alias != NULL; alias++) {
            printf(" - %s\n", *alias);
        }
    }
}

bool is_valid_ip_format(char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0 || inet_pton(AF_INET6, ip, &(sa.sin_addr)) != 0;
}

bool is_valid_domain_name(char *domain) {
    return strlen(domain) > 0 && strcspn(domain, " ") == strlen(domain);
}

int check_option(int option, char *param) {
    if (option == 1) { // Check if it's a valid IP
        return is_valid_ip_format(param);
    } else if (option == 2) { // Check if it's a valid domain
        return is_valid_domain_name(param);
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>

int get_ip(char *hostname, char *ips);
int get_hostname(char *hostname, char *ip);
void print_alternate_names(struct hostent *host_info);
bool is_valid_ip_format(char *ip);
bool is_valid_domain_name(char *domain);
int check_option(int option, char *param);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <option: 1 or 2> <parameter: domain name or IP>\n", argv[0]);
        return 1;
    }

    int option = atoi(argv[1]);
    char *param = argv[2];

    if (check_option(option, param)) {
        if (option == 1) { // Option 1: IP to domain name
            char hostname[100];
            if (get_hostname(hostname, param) == 0) {
                printf("Main domain name: %s\n", hostname);
                char ips[256];
                if (get_ip(hostname, ips) == 0) {
                    printf("Related IP addresses: %s\n", ips);
                } else {
                    printf("No related IP addresses found for: %s\n", hostname);
                }
                struct hostent *host_info = gethostbyname(hostname);
                if (host_info) {
                    print_alternate_names(host_info);
                }
            } else {
                printf("No information found for IP: %s\n", param);
            }
        } else if (option == 2) { // Option 2: domain name to IP
            char ips[256];
            if (get_ip(param, ips) == 0) {
                char *main_ip = strtok(ips, " "); // Main IP
                printf("Official IP: %s\n", main_ip);
                printf("Alias IP:\n");
                
                // Print alias IPs
                char *alias_ip = strtok(NULL, " "); // Start after the main IP
                while (alias_ip != NULL) {
                    printf("%s\n", alias_ip);
                    alias_ip = strtok(NULL, " ");
                }
            } else {
                printf("No information found for domain: %s\n", param);
            }
        } else {
            printf("Invalid option\n");
        }
    } else {
        printf("Invalid option for the given parameter\n");
    }

    return 0;
}

int get_ip(char *hostname, char *ips) {
    struct addrinfo hints, *res;
    int status;
    ips[0] = '\0'; // Initialize the string

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Only allow IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        return 1; // Not found
    }

    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        char ip[INET_ADDRSTRLEN]; // Buffer for IPv4 addresses
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(p->ai_family, &(ipv4->sin_addr), ip, sizeof(ip));
        
        strcat(ips, ip);
        strcat(ips, " ");
    }

    freeaddrinfo(res); // Free the linked list
    return 0;
}

int get_hostname(char *hostname, char *ip) {
    struct in_addr addr;
    inet_aton(ip, &addr); // Convert string to struct in_addr
    struct hostent *es = gethostbyaddr(&addr, sizeof(addr), AF_INET);

    if (es == NULL) {
        return 1; // Not found
    }
    strcpy(hostname, es->h_name);
    return 0;
}

void print_alternate_names(struct hostent *host_info) {
    if (host_info->h_aliases[0] != NULL) {
        printf("Alternate names:\n");
        for (char **alias = host_info->h_aliases; *alias != NULL; alias++) {
            printf(" - %s\n", *alias);
        }
    }
}

bool is_valid_ip_format(char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0; // Only check for IPv4
}

bool is_valid_domain_name(char *domain) {
    return strlen(domain) > 0 && strcspn(domain, " ") == strlen(domain);
}

int check_option(int option, char *param) {
    if (option == 1) { // Check if it's a valid IP
        return is_valid_ip_format(param);
    } else if (option == 2) { // Check if it's a valid domain
        return is_valid_domain_name(param);
    }
    return 0;
}
*/




