/*******************************************
IceAgent internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_WEBRTC_CLIENT_ICE_AGENT__
#define __KINESIS_VIDEO_WEBRTC_CLIENT_ICE_AGENT__

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

#define KVS_ICE_MAX_CANDIDATE_PAIR_COUNT                                1024
#define KVS_ICE_MAX_REMOTE_CANDIDATE_COUNT                              100
#define KVS_ICE_MAX_LOCAL_CANDIDATE_COUNT                               100
#define KVS_ICE_GATHER_REFLEXIVE_AND_RELAYED_CANDIDATE_TIMEOUT          10 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define KVS_ICE_CONNECTIVITY_CHECK_TIMEOUT                              10 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define KVS_ICE_CANDIDATE_NOMINATION_TIMEOUT                            10 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define KVS_ICE_SEND_KEEP_ALIVE_INTERVAL                                15 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define KVS_ICE_TURN_CONNECTION_SHUTDOWN_TIMEOUT                        1 * HUNDREDS_OF_NANOS_IN_A_SECOND
#define KVS_ICE_DEFAULT_TIMER_START_DELAY                               3 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND

// Ta in https://tools.ietf.org/html/rfc8445
#define KVS_ICE_CONNECTION_CHECK_POLLING_INTERVAL                       50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND
#define KVS_ICE_STATE_READY_TIMER_POLLING_INTERVAL                      1 * HUNDREDS_OF_NANOS_IN_A_SECOND
/* Control the calling rate of iceCandidateGatheringTimerTask. Can affect STUN TURN candidate gathering time */
#define KVS_ICE_GATHER_CANDIDATE_TIMER_POLLING_INTERVAL                 50 * HUNDREDS_OF_NANOS_IN_A_MILLISECOND

/* ICE should've received at least one keep alive within this period. Since keep alives are send every 15s */
#define KVS_ICE_ENTER_STATE_DISCONNECTION_GRACE_PERIOD                  2 * KVS_ICE_SEND_KEEP_ALIVE_INTERVAL
#define KVS_ICE_ENTER_STATE_FAILED_GRACE_PERIOD                         15 * HUNDREDS_OF_NANOS_IN_A_SECOND

#define STUN_HEADER_MAGIC_BYTE_OFFSET                                   4

#define KVS_ICE_MAX_ICE_SERVERS                                         3
#define KVS_ICE_MAX_RELAY_CANDIDATE_COUNT                               4
#define KVS_ICE_MAX_NEW_LOCAL_CANDIDATES_TO_REPORT_AT_ONCE              10

// https://tools.ietf.org/html/rfc5245#section-4.1.2.1
#define ICE_PRIORITY_HOST_CANDIDATE_TYPE_PREFERENCE                     126
#define ICE_PRIORITY_SERVER_REFLEXIVE_CANDIDATE_TYPE_PREFERENCE         100
#define ICE_PRIORITY_PEER_REFLEXIVE_CANDIDATE_TYPE_PREFERENCE           110
#define ICE_PRIORITY_RELAYED_CANDIDATE_TYPE_PREFERENCE                  0
#define ICE_PRIORITY_LOCAL_PREFERENCE                                   65535

#define IS_STUN_PACKET(pBuf)                                            (getInt32(*(PUINT32)((pBuf) + STUN_HEADER_MAGIC_BYTE_OFFSET)) == STUN_HEADER_MAGIC_COOKIE)
#define GET_STUN_PACKET_SIZE(pBuf)                                      ((UINT32) getInt16(*(PINT16) ((pBuf) + SIZEOF(UINT16))))

#define IS_CANN_PAIR_SENDING_FROM_RELAYED(p)                            ((p)->local->iceCandidateType == ICE_CANDIDATE_TYPE_RELAYED)

#define KVS_ICE_DEFAULT_TURN_PROTOCOL                                   KVS_SOCKET_PROTOCOL_TCP

#define ICE_HASH_TABLE_BUCKET_COUNT                                     50
#define ICE_HASH_TABLE_BUCKET_LENGTH                                    2

#define ICE_CANDIDATE_ID_LEN                                            8

typedef enum {
    ICE_CANDIDATE_TYPE_HOST             = 0,
    ICE_CANDIDATE_TYPE_PEER_REFLEXIVE   = 1,
    ICE_CANDIDATE_TYPE_SERVER_REFLEXIVE = 2,
    ICE_CANDIDATE_TYPE_RELAYED          = 3,
} ICE_CANDIDATE_TYPE;

typedef enum {
    ICE_CANDIDATE_STATE_NEW,
    ICE_CANDIDATE_STATE_VALID,
    ICE_CANDIDATE_STATE_INVALID,
} ICE_CANDIDATE_STATE;

typedef enum {
    ICE_CANDIDATE_PAIR_STATE_FROZEN         = 0,
    ICE_CANDIDATE_PAIR_STATE_WAITING        = 1,
    ICE_CANDIDATE_PAIR_STATE_IN_PROGRESS    = 2,
    ICE_CANDIDATE_PAIR_STATE_SUCCEEDED      = 3,
    ICE_CANDIDATE_PAIR_STATE_FAILED         = 4,
} ICE_CANDIDATE_PAIR_STATE;

typedef VOID (*IceInboundPacketFunc)(UINT64, PBYTE, UINT32);
typedef VOID (*IceConnectionStateChangedFunc)(UINT64, UINT64);
typedef VOID (*IceNewLocalCandidateFunc)(UINT64, PCHAR);

typedef struct __IceAgent IceAgent;
typedef struct __IceAgent* PIceAgent;

typedef struct {
    UINT64 customData;
    IceInboundPacketFunc inboundPacketFn;
    IceConnectionStateChangedFunc connectionStateChangedFn;
    IceNewLocalCandidateFunc newLocalCandidateFn;
} IceAgentCallbacks, *PIceAgentCallbacks;

typedef struct {
    ICE_CANDIDATE_TYPE iceCandidateType;
    BOOL isRemote;
    KvsIpAddress ipAddress;
    PSocketConnection pSocketConnection;
    ICE_CANDIDATE_STATE state;
    UINT32 priority;
    UINT32 iceServerIndex;
    UINT32 foundation;
    /* If candidate is local and relay, then store the
     * pTurnConnection this candidate is associated to */
    struct __TurnConnection* pTurnConnection;

    /* store pointer to iceAgent to pass it to incomingDataHandler in incomingRelayedDataHandler
     * we pass pTurnConnectionTrack as customData to incomingRelayedDataHandler to avoid look up
     * pTurnConnection every time. */
    PIceAgent pIceAgent;

    /* If candidate is local. Indicate whether candidate
     * has been reported through IceNewLocalCandidateFunc */
    BOOL reported;
    CHAR id[ICE_CANDIDATE_ID_LEN + 1];
} IceCandidate, *PIceCandidate;

typedef struct {
    PIceCandidate local;
    PIceCandidate remote;
    BOOL nominated;
    UINT64 priority;
    ICE_CANDIDATE_PAIR_STATE state;
    PTransactionIdStore pTransactionIdStore;
    UINT64 lastDataSentTime;
    PHashTable requestSentTime;
    UINT64 roundTripTime;
} IceCandidatePair, *PIceCandidatePair;

struct __IceAgent {
    volatile ATOMIC_BOOL agentStartGathering;
    volatile ATOMIC_BOOL remoteCredentialReceived;
    volatile ATOMIC_BOOL candidateGatheringFinished;
    volatile ATOMIC_BOOL shutdown;
    volatile ATOMIC_BOOL restart;
    volatile ATOMIC_BOOL processStun;

    CHAR localUsername[MAX_ICE_CONFIG_USER_NAME_LEN + 1];
    CHAR localPassword[MAX_ICE_CONFIG_CREDENTIAL_LEN + 1];
    CHAR remoteUsername[MAX_ICE_CONFIG_USER_NAME_LEN + 1];
    CHAR remotePassword[MAX_ICE_CONFIG_CREDENTIAL_LEN + 1];
    CHAR combinedUserName[MAX_ICE_CONFIG_USER_NAME_LEN + 1];

    PDoubleList localCandidates;
    PDoubleList remoteCandidates;
    // store PIceCandidatePair which will be immediately checked for connectivity when the timer is fired.
    PStackQueue triggeredCheckQueue;
    PDoubleList iceCandidatePairs;

    PConnectionListener pConnectionListener;
    BOOL isControlling;
    UINT64 tieBreaker;

    MUTEX lock;

    // timer tasks
    UINT32 iceAgentStateTimerTask;
    UINT32 keepAliveTimerTask;
    UINT32 iceCandidateGatheringTimerTask;

    // Current ice agent state
    UINT64 iceAgentState;
    // The state machine
    PStateMachine pStateMachine;
    STATUS iceAgentStatus;
    UINT64 stateEndTime;
    UINT64 candidateGatheringEndTime;
    PIceCandidatePair pDataSendingIceCandidatePair;

    IceAgentCallbacks iceAgentCallbacks;

    IceServer iceServers[KVS_ICE_MAX_ICE_SERVERS];
    UINT32 iceServersCount;

    KvsIpAddress localNetworkInterfaces[MAX_LOCAL_NETWORK_INTERFACE_COUNT];
    UINT32 localNetworkInterfaceCount;

    UINT32 foundationCounter;

    UINT32 relayCandidateCount;

    TIMER_QUEUE_HANDLE timerQueueHandle;

    UINT64 lastDataReceivedTime;
    BOOL detectedDisconnection;
    UINT64 disconnectionGracePeriodEndTime;

    ICE_TRANSPORT_POLICY iceTransportPolicy;
    KvsRtcConfiguration kvsRtcConfiguration;

    // Pre-allocated stun packets
    PStunPacket pBindingIndication;
    PStunPacket pBindingRequest;

    // store transaction ids for stun binding request.
    PTransactionIdStore pStunBindingRequestTransactionIdStore;
};


//////////////////////////////////////////////
// internal functions
//////////////////////////////////////////////

/**
 * allocate the IceAgent struct and store username and password
 *
 * @param - PCHAR - IN - username
 * @param - PCHAR - IN - password
 * @param - PIceAgentCallbacks - IN - callback for inbound packets
 * @param - PRtcConfiguration - IN - RtcConfig
 * @param - PIceAgent* - OUT - the created IceAgent struct
 *
 * @return - STATUS - status of execution
 */
STATUS createIceAgent(PCHAR, PCHAR, PIceAgentCallbacks, PRtcConfiguration, TIMER_QUEUE_HANDLE, PConnectionListener, PIceAgent*);

/**
 * deallocate the PIceAgent object and all its resources.
 *
 * @return - STATUS - status of execution
 */
STATUS freeIceAgent(PIceAgent*);

/**
 * if PIceCandidate doesnt exist already in remoteCandidates, create a copy and add to remoteCandidates
 *
 * @param - PIceAgent - IN - IceAgent object
 * @param - PIceCandidate - IN - new remote candidate to add
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentAddRemoteCandidate(PIceAgent, PCHAR);

/**
 * Initiates stun communication with remote candidates.
 *
 * @param - PIceAgent - IN - IceAgent object
 * @param - PCHAR - IN - remote username
 * @param - PCHAR - IN - remote password
 * @param - BOOL - IN - is controlling agent
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentStartAgent(PIceAgent, PCHAR, PCHAR, BOOL);

/**
 * Initiates candidate gathering
 *
 * @param - PIceAgent - IN - IceAgent object
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentStartGathering(PIceAgent);

/**
 * Serialize a candidate for Trickle ICE or exchange via SDP
 *
 * @param - PIceAgent - IN - IceAgent object
 * @param - PCHAR - OUT - Destination buffer
 * @param - UINT32 - OUT - Size of destination buffer
 *
 * @return - STATUS - status of execution
 */
STATUS iceCandidateSerialize(PIceCandidate, PCHAR, PUINT32);

/**
 * Send data through selected connection. PIceAgent has to be in ICE_AGENT_CONNECTION_STATE_CONNECTED state.
 *
 * @param - PIceAgent - IN - IceAgent object
 * @param - PBYTE - IN - buffer storing the data to be sent
 * @param - UINT32 - IN - length of data
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentSendPacket(PIceAgent, PBYTE, UINT32);

/**
 * gather local ip addresses and create a udp port. If port creation succeeded then create a new candidate
 * and store it in localCandidates. Ips that are already a local candidate will not be added again.
 *
 * @param - PIceAgent - IN - IceAgent object
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentInitHostCandidate(PIceAgent);

/**
 * Starting from given index, fillout PSdpMediaDescription->sdpAttributes with serialize local candidate strings.
 *
 * @param - PIceAgent - IN - IceAgent object
 * @param - PSdpMediaDescription - IN - PSdpMediaDescription object whose sdpAttributes will be filled with local candidate strings
 * @param - UINT32 - IN - buffer length of pSdpMediaDescription->sdpAttributes[index].attributeValue
 * @param - PUINT32 - IN - starting index in sdpAttributes
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentPopulateSdpMediaDescriptionCandidates(PIceAgent, PSdpMediaDescription, UINT32, PUINT32);

/**
 * Start shutdown sequence for IceAgent. Once the function returns Ice will not deliver anymore data and
 * IceAgent is ready to be freed. User should stop calling iceAgentSendPacket after iceAgentShutdown returns.
 * iceAgentShutdown is idempotent.
 *
 * @param - PIceAgent - IN - IceAgent object
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentShutdown(PIceAgent);

/**
 * Restart IceAgent. IceAgent is reset back to the same state when it was first created. Once iceAgentRestart() return,
 * call iceAgentStartGathering() to start gathering and call iceAgentStartAgent() to give iceAgent the new remote uFrag
 * and uPwd. While Ice is restarting, iceAgentSendPacket can still be called to send data if a connected pair exists.
 *
 * @param - PIceAgent - IN - IceAgent object
 * @param - PCHAR - IN - new local uFrag
 * @param - PCHAR - IN - new local uPwd
 *
 * @return - STATUS - status of execution
 */
STATUS iceAgentRestart(PIceAgent, PCHAR, PCHAR);

STATUS iceAgentReportNewLocalCandidate(PIceAgent, PIceCandidate);
STATUS iceAgentValidateKvsRtcConfig(PKvsRtcConfiguration);

// Incoming data handling functions
STATUS incomingDataHandler(UINT64, PSocketConnection, PBYTE, UINT32, PKvsIpAddress, PKvsIpAddress);
STATUS incomingRelayedDataHandler(UINT64, PSocketConnection, PBYTE, UINT32, PKvsIpAddress, PKvsIpAddress);
STATUS handleStunPacket(PIceAgent, PBYTE, UINT32, PSocketConnection, PKvsIpAddress, PKvsIpAddress);

// IceCandidate functions
STATUS updateCandidateAddress(PIceCandidate, PKvsIpAddress);
STATUS findCandidateWithIp(PKvsIpAddress, PDoubleList, PIceCandidate*);
STATUS findCandidateWithSocketConnection(PSocketConnection, PDoubleList, PIceCandidate*);

// IceCandidatePair functions
STATUS createIceCandidatePairs(PIceAgent, PIceCandidate, BOOL);
STATUS freeIceCandidatePair(PIceCandidatePair*);
STATUS insertIceCandidatePair(PDoubleList, PIceCandidatePair);
STATUS findIceCandidatePairWithLocalSocketConnectionAndRemoteAddr(PIceAgent, PSocketConnection, PKvsIpAddress, BOOL, PIceCandidatePair*);
STATUS pruneUnconnectedIceCandidatePair(PIceAgent);
STATUS iceCandidatePairCheckConnection(PStunPacket, PIceAgent, PIceCandidatePair);

STATUS iceAgentSendSrflxCandidateRequest(PIceAgent);
STATUS iceAgentCheckCandidatePairConnection(PIceAgent);
STATUS iceAgentSendCandidateNomination(PIceAgent);
STATUS iceAgentSendStunPacket(PStunPacket, PBYTE, UINT32, PIceAgent, PIceCandidate, PKvsIpAddress);

STATUS iceAgentInitHostCandidate(PIceAgent);
STATUS iceAgentInitSrflxCandidate(PIceAgent);
STATUS iceAgentInitRelayCandidates(PIceAgent);
STATUS iceAgentInitRelayCandidate(PIceAgent, PKvsIpAddress, UINT32, KVS_SOCKET_PROTOCOL);

STATUS iceAgentCheckConnectionStateSetup(PIceAgent);
STATUS iceAgentConnectedStateSetup(PIceAgent);
STATUS iceAgentNominatingStateSetup(PIceAgent);
STATUS iceAgentReadyStateSetup(PIceAgent);

// timer callbacks. timer callbacks are interlocked by time queue lock.
STATUS iceAgentStateTransitionTimerCallback(UINT32, UINT64, UINT64);
STATUS iceAgentSendKeepAliveTimerCallback(UINT32, UINT64, UINT64);
STATUS iceAgentGatherCandidateTimerCallback(UINT32, UINT64, UINT64);

// Default time callback for the state machine
UINT64 iceAgentGetCurrentTime(UINT64);

STATUS iceAgentNominateCandidatePair(PIceAgent);
STATUS iceAgentInvalidateCandidatePair(PIceAgent);
STATUS iceAgentCheckPeerReflexiveCandidate(PIceAgent, PKvsIpAddress, UINT32, BOOL, PSocketConnection);
STATUS iceAgentFatalError(PIceAgent, STATUS);
VOID iceAgentLogNewCandidate(PIceCandidate);

UINT32 computeCandidatePriority(PIceCandidate);
UINT64 computeCandidatePairPriority(PIceCandidatePair, BOOL);
PCHAR iceAgentGetCandidateTypeStr(ICE_CANDIDATE_TYPE);

#ifdef  __cplusplus
}
#endif
#endif  /* __KINESIS_VIDEO_WEBRTC_CLIENT_ICE_AGENT__ */
