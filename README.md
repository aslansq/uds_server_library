# uds_server_library demos

It contains demo configuration for both bootloader and application, showcasing how UDS services can be integrated and tested in different firmware contexts.

## Supported UDS Services

The library currently supports the following UDS (Unified Diagnostic Services) as part of its core functionality:

| Service Name                | Service ID | Description                        |
|-----------------------------|------------|------------------------------------|
| Diagnostic Session Control  | 0x10       | Switches between diagnostic modes  |
| ECU Reset                   | 0x11       | Resets the ECU                     |
| Security Access             | 0x27       | Handles security unlocking         |
| Tester Present              | 0x3E       | Keeps diagnostic session alive     |
| Routine Control             | 0x31       | Executes routines on the ECU       |
| Write Data By Identifier    | 0x2E       | Writes data to specific IDs        |
| Request Download            | 0x34       | Initiates data download            |
| Transfer Data               | 0x36       | Transfers data blocks              |
| Request Transfer Exit       | 0x37       | Ends data transfer session         |

Additional services can be integrated as needed.

## Hardware Requirements

The UDS implementation itself is not hardware dependent and can be adapted to various platforms. The provided examples use the Nucleo-C092RC development board, but you can port the code to other hardware as needed.

## Author & Support

Implementation solely written by me. Contact to get library for your specific ECU.
mehmet.aslan@asuds.com
