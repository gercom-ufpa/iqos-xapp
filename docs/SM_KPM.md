# Service Model KPM_V3_00

## Supported Report Style Types
- **RIC Style Type 1**:  E2 Node Measurement. Used to carry measurement report from a target E2 Node. More in details,
                    it contains measurement types that Near-RT RIC is requesting to subscribe followed by a list 
                    of subcounters to be measured for each measurement type, and a granularity period
                    indicating collection interval of those measurements.


- **RIC Style Type 2**: Used to carry measurement report for a single UE of interest from a target E2 Node.


- **RIC Style Type 3**: Used to carry UE-level measurement report for a group of UEs per measurement type matching subscribed conditions from a target E2 Node.


- **RIC Style Type 4**: Used to carry measurement report for a group of UEs across a set of measurement types satisfying common subscribed conditions from a target E2 Node


- **RIC Style Type 5**: Used to carry measurement report for multiple UE of interest from a target E2 Node
