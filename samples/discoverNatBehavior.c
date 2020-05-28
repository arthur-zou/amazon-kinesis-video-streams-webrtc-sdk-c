#include <com/amazonaws/kinesis/video/webrtcclient/Include.h>

#define NETWORK_INTERFACE_NAME_PARAM                "-i"
#define STUN_HOSTNAME_PARAM                         "-s"
#define DEFAULT_STUN_HOST                           "stun:stun.sipgate.net:3478"

BOOL filterFunc(UINT64 data, PCHAR name)
{
    CHAR *desiredInterface = (CHAR *) data;
    if (desiredInterface == NULL || STRCMP(name, desiredInterface) == 0) {
        return TRUE;
    }
    return FALSE;
}

INT32 main(INT32 argc, CHAR **argv)
{
    PCHAR interfaceName = NULL, stunHostname = NULL;
    printf("Usage: ./discoverNatBehavior -i network-interface-name -s stun-hostname\n");
    NAT_BEHAVIOR mappingBehavior = NAT_BEHAVIOR_NONE, filteringBehavior = NAT_BEHAVIOR_NONE;
    INT32 i;

    for (i = 1; i < argc; ++i) {
        if (STRCMP(argv[i], NETWORK_INTERFACE_NAME_PARAM) == 0) {
            interfaceName = argv[++i];
            i++;
        } else if (STRCMP(argv[i], STUN_HOSTNAME_PARAM) == 0) {
            stunHostname = argv[++i];
            i++;
        } else {
            printf("Unknown param %s", argv[i]);
        }
    }

    if (stunHostname == NULL) {
        stunHostname = DEFAULT_STUN_HOST;
    }

    printf("Using stun host: %s, local network interface %s.\n", stunHostname, interfaceName);

    initKvsWebRtc();
    SET_LOGGER_LOG_LEVEL(LOG_LEVEL_DEBUG);

    if (discoverNatBehavior(stunHostname, &mappingBehavior, &filteringBehavior, filterFunc, (UINT64) interfaceName)) {
        fprintf(stderr, "Failed to detect NAT behavior\n");
        exit(1);
    }

    printf("Detected NAT mapping behavior %s\n", getNatBehaviorStr(mappingBehavior));
    switch (mappingBehavior) {
        case NAT_BEHAVIOR_NONE:
            printf("Failed to detect NAT mapping behavior");
            break;
        case NAT_BEHAVIOR_NOT_BEHIND_ANY_NAT:
            printf("Host is not behind any NAT. Its IP address is the public IP address. STUN is not needed.\n");
            break;
        case NAT_BEHAVIOR_NO_UDP_CONNECTIVITY:
            printf("Host does not have any UDP connectivity. STUN is not usable. Can only connect using TCP relay\n");
            break;
        case NAT_BEHAVIOR_ENDPOINT_INDEPENDENT:
            printf("Host's NAT uses same public IP address regardless of destination address. STUN is usable.\n");
            break;
        case NAT_BEHAVIOR_ADDRESS_DEPENDENT:
            printf("Host's NAT uses different public IP address for different destination address. STUN is not usable.\n");
            break;
        case NAT_BEHAVIOR_PORT_DEPENDENT:
            printf("Host's NAT uses different public IP address for different destination address and port. STUN is not usable.\n");
            break;
    }

    printf("Detected NAT filtering behavior %s\n", getNatBehaviorStr(filteringBehavior));
    switch (filteringBehavior) {
        case NAT_BEHAVIOR_NONE:
            printf("Failed to detect NAT filtering behavior\n");
            break;
        case NAT_BEHAVIOR_NOT_BEHIND_ANY_NAT:
            printf("Host is not behind any NAT. Its IP address is the public IP address. STUN is not needed.\n");
            break;
        case NAT_BEHAVIOR_NO_UDP_CONNECTIVITY:
            printf("Host does not have any UDP connectivity. STUN is not usable. Can only connect using TCP relay\n");
            break;
        case NAT_BEHAVIOR_ENDPOINT_INDEPENDENT:
            printf("Host's NAT allows to receive UDP packet from any external address. STUN is usable.\n");
            break;
        case NAT_BEHAVIOR_ADDRESS_DEPENDENT:
            printf("Host's NAT allows to receive UDP packet from external address that host had previously sent data to. STUN is usable.\n");
            break;
        case NAT_BEHAVIOR_PORT_DEPENDENT:
            printf("Host's NAT allows to receive UDP packet from external address and port that host had previously sent data to. STUN is usable.\n");
            break;
    }

    deinitKvsWebRtc();

    return 0;
}