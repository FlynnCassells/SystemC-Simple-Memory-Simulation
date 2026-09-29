#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <cstdlib>

using namespace sc_core;
using namespace tlm;

SC_MODULE(Memory) {
    tlm_utils::simple_target_socket<Memory> socket;
    std::vector<uint8_t> mem;
    sc_time latency;
    SC_CTOR(Memory) : socket("socket"), mem(4096, 0), latency(50, SC_NS) {
        socket.register_b_transport(this, &Memory::b_transport);
    }
    void b_transport(tlm_generic_payload& trans, sc_time& delay) {
        tlm_command command = trans.get_command();
        sc_dt::uint64 address = trans.get_address();
        unsigned char* dataPtr = trans.get_data_ptr();
        unsigned int dataLength = trans.get_data_length();
        if (mem.size() < address + dataLength) {
            trans.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }
        if(command== TLM_READ_COMMAND){
            std::memcpy(dataPtr, &mem[address], dataLength);
        } else if (command == TLM_WRITE_COMMAND) {
            std::memcpy(&mem[address], dataPtr, dataLength);
        }
        delay += latency;
        trans.set_response_status(TLM_OK_RESPONSE);
    }
};

SC_MODULE(TrafficGen) {
    tlm_utils::simple_initiator_socket<TrafficGen> socket;
    SC_CTOR(TrafficGen) : socket("socket"){
        SC_THREAD(run);
    }
    void run(){
        std::cout << "The current simulated time is " << sc_time_stamp() << std::endl;
        tlm_generic_payload payload;
        const unsigned char validation_data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
        unsigned char buffer[4] = {0xDE, 0xAD, 0xBE, 0xEF};
        payload.set_command(TLM_WRITE_COMMAND);
        payload.set_address(0x10);
        payload.set_data_ptr(buffer);
        payload.set_data_length(4);
        payload.set_streaming_width(4);
        payload.set_byte_enable_ptr(nullptr);
        payload.set_response_status(TLM_INCOMPLETE_RESPONSE);
        sc_time delay = SC_ZERO_TIME;
        socket->b_transport(payload, delay);
        wait(delay);
        if (payload.is_response_error()) {
            std::cout << "An error occured when writing the data" << std::endl;
        } else {
            std::cout << "successfully wrote data at simulation time " << sc_time_stamp() << std::endl;
        }
        unsigned char buffer_check[4] = {0,0,0,0};
        payload.set_command(TLM_READ_COMMAND);
        payload.set_data_ptr(buffer_check);
        payload.set_response_status(TLM_INCOMPLETE_RESPONSE);
        delay = SC_ZERO_TIME;
        socket->b_transport(payload, delay);
        wait(delay);
        if (payload.is_response_error()) {
            std::cout << "An error occured when reading the data" << std::endl;
        } else {
            std::cout << "successfully read data at simulation time " << sc_time_stamp() << std::endl;
        }
        if(std::memcmp(validation_data, buffer_check, 4)==0){
            std::cout << "successfully validated data transfer at " << sc_time_stamp() << std::endl;
        } else {
            std::cout << "the validation check failed, there was an error with the data transfer" << std::endl;
        }
        const unsigned int NUM_TRANSACTIONS = 1000;
        const unsigned int BURST_BYTES = 64;
        unsigned char burst[BURST_BYTES] = {};
        payload.set_command(TLM_WRITE_COMMAND);
        payload.set_data_length(BURST_BYTES);
        payload.set_streaming_width(BURST_BYTES);
        payload.set_byte_enable_ptr(nullptr);
        payload.set_data_ptr(burst);
        unsigned int error_count = 0;
        sc_time start = sc_time_stamp();
        for (unsigned int i = 0; i < NUM_TRANSACTIONS; i++) {
            payload.set_address((i*BURST_BYTES)%4096);
            payload.set_response_status(TLM_INCOMPLETE_RESPONSE);
            delay = SC_ZERO_TIME;
            socket->b_transport(payload, delay);
            wait(delay);
            if (payload.is_response_error()) {
                error_count++;
            }
        }   
        sc_time end = sc_time_stamp();
        double elapsed_time = (end - start).to_seconds();
        double bandwidth = (BURST_BYTES*NUM_TRANSACTIONS)/elapsed_time * 1e-9;
        std::cout << "Simulated a total of " << BURST_BYTES*NUM_TRANSACTIONS << " bytes givng a bandwith of " << bandwidth 
        << " GB/s, and an average latency of " << elapsed_time/NUM_TRANSACTIONS*1e9 << " ns. The simulation completed with " << error_count << " errors." << std::endl;
    }
};

int sc_main(int argc, char *argv[]){
    Memory mem("mem");
    if (argc > 1) {
        mem.latency = sc_time(std::atof(argv[1]), SC_NS);
    }
    TrafficGen gen("gen");
    gen.socket.bind(mem.socket); 
    sc_start();
    return 0;
}