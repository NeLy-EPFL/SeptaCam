/*
 *       Example of AP_TimeCheck library.
 *       Code by Randy Mackay. DIYDrones.com
 */

#include <FastSerial.h>
#include <AP_Common.h>
#include <AP_TimeCheck.h>

// declare serial ports
FastSerialPort0(Serial);

// AP_TimeCheck_test.pde
//	tests useage of AP_TimeCheck library to record rough timings of loops

// our target loop time is 10,000 micros (i.e. 100hz)
AP_TimeCheck time_checker(10000,100);

// loop timers
uint32_t fast_loop;
uint32_t print_loop;
uint16_t delay_count = 0;

// setup
void setup(void)
{
	// initialise serial port
    Serial.begin(115200);

	// print welcome message
    Serial.println("AP_TimeCheck library tester ver 0.1");

	// initialise loop timer
	reset_timers();
}

// reset_timers - resets timers
void reset_timers()
{
	// initialise loop timer
	fast_loop = micros();
	print_loop = millis();
}

// main loop
void loop(void)
{
	uint32_t current_time_ms = millis();
	uint32_t current_time_micros = micros();
	uint32_t time_diff;
	uint16_t a_random_number;

	// run fast loop (100hz)
	time_diff = current_time_micros - fast_loop;
	if( time_diff > 10000 ) {
		// reset loop timer
		fast_loop = current_time_micros;

		// record time
		time_checker.addTime(time_diff);

		// add a delay
		delayMicroseconds(100);
	}

	// dump results to console every 10 seconds
	if( current_time_ms - print_loop > 10000 ) {
		print_loop = current_time_ms;
		// display results
		time_checker.dump_results();
		// clear results
		time_checker.clear();
		// reset timers (so we don't record the time to dump the results)
		reset_timers();
	}
}

