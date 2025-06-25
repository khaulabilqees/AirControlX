# Air Traffic Control System (ATCS) - OS semester project

A multithreaded, multi-process C++ simulation of an Air Traffic Control System (ATCS) using forks, mutexes, inter-process communication via named pipes (FIFOs), threads. The system handles airline violations, automatic violation notices (AVNs), payment processing, and portal tracking.
atcs class, the general control system, contains all airlines and each airline class contains its respective flights
---

##  Features

- a total of 3 runways ( A, B, C) 
- Simulates flights with commercial, emergency and cargo types 
- During Flight simulation, each flight is allocated a runway based on its direction and type
- Mutexes used to ensure each runway is occupied by only one flight at a time
- Monitors each flight across critical flight phases (flight can either be 
- Detects speed violations and generates AVNs and sends them to the avnController process
- Once simulation finishes sends all generated AVNs for payment processing via a simulated StripePay process
- Reflects payment status in a dedicated Airline Portal by sending back the status to the airlinePortal process to confirm payment
- Uses multiple child processes and named pipes for inter-process communication
- Cleanup of child processes and FIFOs

---
## Process Breakdown

## AVN Generator (`avnController()`)

**Type:** Child Process in main (parent process) 
**Pipe Used:** `violationPipe` (input), `avnResultsPipe` (output)

**Responsibilities:**
- Reads `ViolationRecord` data from `violationPipe`
- Constructs an `AVN` (Air Violation Notice) with:
  - Flight info, airline, reason, timestamp
  - Fine amount based on flight type 
- Sends the AVN to:
  - `avnResultsPipe` (read by the parent thread)
  - (optionally) `avnToAirline` for airline-specific portals

## StripePay Processor (`stripePayProcess()`)

**Type:** Child Process in main (parent process)
**Pipe Used:** `AirlineToPayment` (input), `paymentToAirlinePortal` (output)

**Responsibilities:**
- Receives unpaid AVNs from the parent process via `AirlineToPayment`
- Simulates successful payment (for simplicity)
- Updates the AVN's `paid` field to `true`
- Sends the updated AVN to `paymentToAirlinePortal`

## Airline Portal (`airlinePortalController()`)

**Type:** Child Process in main (parent process)
**Pipe Used:** `paymentToAirlinePortal` (input)

**Responsibilities:**
- reads payment updates from StripePay
- Displays the updated paid statuses of AVNs 
