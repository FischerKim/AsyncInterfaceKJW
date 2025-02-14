#pragma once
#pragma pack(push,1)
namespace II
{
#define RX_STACK_SIZE	50000
#define TX_STACK_SIZE	50000
#define RX_TASK_PRIORITY	100
#define TX_TASK_PRIORITY	100
#define MAX_TX_MSGS		3000
#define MAX_TX_MSGLEN	3000
#define MAX_RX_MSGLEN	10000

#define UDP_RX_STACK_SIZE	50000
#define UDP_TX_STACK_SIZE	50000
#define UDP_RX_TASK_PRIORITY	100
#define UDP_TX_TASK_PRIORITY	100
#define UDP_MAX_TX_MSGS		3000
#define UDP_MAX_TX_MSGLEN	3000
#define UDP_MAX_RX_MSGLEN	10000

#define SERIAL_RX_STACK_SIZE	50000
#define SERIAL_TX_STACK_SIZE	100000
#define SERIAL_RX_TASK_PRIORITY	95
#define SERIAL_TX_TASK_PRIORITY	96
#define SERIAL_MAX_TX_MSGS		1000
#define SERIAL_MAX_TX_MSGLEN	3000
#define SERIAL_MAX_RX_MSGLEN	3000
#define SERIAL_MAX_RX_BUFSIZE	30000

#define DATAWID_8				8
//#define EW_STOPBIT			"one" //one,  onepointfive,  two
//#define EW_PARITY				"none" //none, odd, even
#define SERIAL_RX_BUFFER_SIZE	500
#define SERIAL_RX_BUFFER_NUMBER	20
#define SERIAL_TX_BUFFER_SIZE	500
#define SERIAL_TX_BUFFER_NUMBER	20
#define EW_BAUDRATE				38400

// [INTERFACE_ID : The interface ID is designated as a UNIQUE NUMBER according to the order in which it was created.
// [IF_TYPE : UDP(0), MULTICAST(1), TCP_SERVER(2), TCP_CLIENT(3), SERIAL(4)]

    namespace network
    {
        namespace modules
        {
            #define MAX_EVENTS 10
            #define BUFFER_SIZE 1024
            //static const int _reconnect_time_ = 100;
        }

        struct session_info
        {
            // ����
            short		    _id;				// OG, NAV, CMS �� ���� �ĺ� ��ȣ��� session id
            short		    _type;			    // UDP, MULTICAST, TCP(S,C), SERIAL �� ��� ��� Ÿ��
            char		    _description[20];   // ��� ��� ����

            // Ethernet
            char		    _source_ip[16]; // �۽��� IP
            unsigned int    _source_port; // �۽��� ��Ʈ
            char		    _destination_ip[16]; // ������ IP
            unsigned int	_destination_port; // ������ ��Ʈ

            // Serial
            char		    _serial_port[10]; // �ø��� ��Ʈ
            unsigned int	_baud_rate; // baud rate
            unsigned int	_data_width; // data width
            char		    _stop_bits[15]; // stop bits
            char		    _parity_mode[10]; // parity mode

            session_info() // ��� ��� ������
                : _id(0),
                _type(0),
                _source_port(0),
                _destination_port(0),
                _baud_rate(0),
                _data_width(0)
            {
                std::fill(std::begin(_description), std::end(_description), 0);
                std::fill(std::begin(_source_ip), std::end(_source_ip), 0);
                std::fill(std::begin(_destination_ip), std::end(_destination_ip), 0);
                std::fill(std::begin(_serial_port), std::end(_serial_port), 0);
                std::fill(std::begin(_stop_bits), std::end(_stop_bits), 0);
                std::fill(std::begin(_parity_mode), std::end(_parity_mode), 0);
            }
        };

        struct tcp_info  //TCP ���� ����
        {
            short		        _id;
            unsigned int        _rx_stack_size;
            unsigned int        _tx_stack_size;
            unsigned int        _rx_priority;
            unsigned int        _tx_priority;
            unsigned int        _max_tx_msg_count;
            unsigned int        _max_tx_msg_length;
            unsigned int        _max_rx_msg_length;

            char                _name[16];
            char                _source_ip[16];
            unsigned int        _source_port;
            char                _destination_ip[16];
            unsigned int        _destination_port;


            tcp_info() // ������
                : _id(0), 
                _rx_stack_size(RX_STACK_SIZE),
                _tx_stack_size(TX_STACK_SIZE),
                _rx_priority(RX_TASK_PRIORITY),
                _tx_priority(TX_TASK_PRIORITY),
                _max_tx_msg_count(MAX_TX_MSGS),
                _max_tx_msg_length(MAX_TX_MSGLEN),
                _max_rx_msg_length(MAX_RX_MSGLEN),
                _source_port(0),
                _destination_port(0)
            {
                std::fill(std::begin(_name), std::end(_name), 0);
                std::fill(std::begin(_source_ip), std::end(_source_ip), 0);
                std::fill(std::begin(_destination_ip), std::end(_destination_ip), 0);
            }
        };

        struct udp_info // UDP ���� ����
        {
            short		        _id;
            unsigned int        _rx_stack_size;
            unsigned int        _tx_stack_size;
            unsigned int        _rx_priority;
            unsigned int        _tx_priority;
            unsigned int        _max_tx_msg_count;
            unsigned int        _max_tx_msg_length;
            unsigned int        _max_rx_msg_length;
            unsigned int        _type;

            char                _name[16];
            char                _source_ip[16];
            unsigned int        _source_port;
            char                _destination_ip[16];
            unsigned int        _destination_port;
            unsigned int        _ttl;

            udp_info() // ������
                : _id(0), 
                _rx_stack_size(UDP_RX_STACK_SIZE),
                _tx_stack_size(UDP_TX_STACK_SIZE),
                _rx_priority(UDP_RX_TASK_PRIORITY),
                _tx_priority(UDP_TX_TASK_PRIORITY),
                _max_tx_msg_count(UDP_MAX_TX_MSGS),
                _max_tx_msg_length(UDP_MAX_TX_MSGLEN),
                _max_rx_msg_length(UDP_MAX_RX_MSGLEN),
                _source_port(0),
                _destination_port(0),
                _ttl(1)
            {
                std::fill(std::begin(_name), std::end(_name), 0);
                std::fill(std::begin(_source_ip), std::end(_source_ip), 0);
                std::fill(std::begin(_destination_ip), std::end(_destination_ip), 0);
            }
        };

        struct serial_info // �ø��� ��� ���� ����
        {
            short		        _id;
            char                _name[10];

            unsigned int        _rx_stack_size;
            unsigned int        _tx_stack_size;
            unsigned int        _rx_priority;
            unsigned int        _tx_priority;
            unsigned int        _max_tx_msg_count;
            unsigned int        _max_tx_msg_length;
            unsigned int        _max_rx_msg_length;

            unsigned int        _baud_rate;
            unsigned int        _data_width;
            char                _stop_bits[15];
            char                _parity_mode[10];

            serial_info() // ������
                : _id(0), 
                _rx_stack_size(SERIAL_RX_STACK_SIZE),
                _tx_stack_size(SERIAL_TX_STACK_SIZE),
                _rx_priority(SERIAL_RX_TASK_PRIORITY),
                _tx_priority(SERIAL_TX_TASK_PRIORITY),
                _max_tx_msg_count(SERIAL_MAX_TX_MSGS),
                _max_tx_msg_length(SERIAL_MAX_TX_MSGLEN),
                _max_rx_msg_length(SERIAL_MAX_RX_MSGLEN),
                _data_width(DATAWID_8)
            {
                std::fill(std::begin(_stop_bits), std::end(_stop_bits), 0);
                std::fill(std::begin(_parity_mode), std::end(_parity_mode), 0);
                //strcpy(_stop_bits, "one");
                //strcpy(_parity_mode, "none");
            }
        };
    }
}

#pragma pack(pop)