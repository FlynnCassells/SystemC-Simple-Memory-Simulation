A simple SystemC/TLM-2.0 memory target and traffic generator, loosely-timed using b_transport. 
In this model, a write then read validation is performed to ensure the memory is working as expected. 
It further be implemented in the terminal with the command "for lat in 10 25 50 100 200; do ./main $lat; done" to perform a design space sweep, and compare the bandwidth for various latencies.
