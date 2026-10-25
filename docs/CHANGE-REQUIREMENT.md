Change Requirement: Minimum-Bit Data Representation
The robot car firmware must minimise the number of bits used to represent application data. Each data field representing application information must use only the number of bits required to represent its valid range, using C bit-fields or explicit masks and shifts where appropriate. Examples include:
•	A value ranging from 1 to 5: 3 bits
•	A state machine with up to 8 states: 3 bits
•	Fault conditions such as motor stall, encoder fault, IMU fault, no echo and Wi-Fi loss: use individual bits within a shared fault bitmask, rather than separate integer variables.
The requirement applies to application data throughout the firmware, including:
1.	motion control and encoder data;
2.	line and barcode sensing;
3.	obstacle detection;
4.	hump estimation;
5.	vehicle and controller states;
6.	data shared between tasks or between an ISR and a task;
7.	inter-task queue messages; and
8.	MQTT telemetry and command payloads.
For MQTT communication, application data should be represented using compact binary fields rather than sending individual values as text.
When multiple values are packed into the same storage unit and that data is shared between tasks, or between an ISR and a task, access must be implemented safely. Read-modify-write operations on shared packed data must not introduce race conditions, and atomic access or appropriate synchronisation must be used where required.

Week 13 Documentation and Demonstration
As part of the Week 13 final submission, teams must include the following evidence and documentation in their GitHub repository:
1.	Data representation table
Identify each relevant application data field, its valid range, required number of bits, and how it is represented in the implementation.
2.	RAM usage comparison
Measure and compare RAM usage before and after implementing the change.
3.	Message size comparison
Measure and compare the size of relevant inter-task and/or communication messages before and after implementing the change.
4.	Correctness and performance
Provide evidence that the revised representation causes no loss of required functionality and no unacceptable degradation in timing or system performance.
