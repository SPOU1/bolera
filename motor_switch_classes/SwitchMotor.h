#pragma once

class SwitchMotor {
	public:
	enum SwState {
		MOVING = 0,
		SIDE_1 = 1,
		SIDE_2 = 2
		};
	
	private:
	volatile uint8_t* pinReg;
	volatile uint8_t* portReg;
	volatile uint8_t* ddrReg;
	uint8_t mask;
	
	volatile SwState currentState;
	volatile bool flagPressed;
	bool expectedDirection;
	
	uint32_t lastDebounceTimestamp;
	bool lastReading;
	bool stableState;
	const uint32_t debounceDelay = 20;
	
	public:
	SwitchMotor(volatile uint8_t* pinR, volatile uint8_t* portR, volatile uint8_t* ddrR, uint8_t m) {
		pinReg  = pinR;
		portReg = portR;
		ddrReg  = ddrR;
		mask    = m;
		currentState = MOVING;
		flagPressed = false;
		expectedDirection = false;
		
		lastDebounceTimestamp = 0;
		lastReading = false;
		stableState = false;
	}
	
	void init() {
		*ddrReg &= ~mask;	// Input
		*portReg |= mask;	// Pull-up
	}
	
	bool isPressed() {
		return !(*pinReg & mask);	// Pressed is 0
	}

	SwState getState() {
		return currentState;
	}
	
	void update(uint32_t currentTime) {
		bool reading = isPressed();
		
        if (reading != lastReading) {
			lastDebounceTimestamp = currentTime;
		}
		
		if ((currentTime - lastDebounceTimestamp) > debounceDelay) {
			if (reading != stableState) {
				stableState = reading;
				
				if (stableState) {
					flagPressed = true;
					if (expectedDirection) {
						currentState = SIDE_1;
					} else {
						currentState = SIDE_2;
					}
				}
			}
		}
		lastReading = reading;
    }
	
	void forceState(SwState s) {
		currentState = s;
		flagPressed = false;
	}
	
	void setExpectedDirection(bool direction) {
		expectedDirection = direction;
		flagPressed = false;
	}
	
	bool consumePressedFlag() {
		if (flagPressed) {
			flagPressed = false;
			return true;
		}
		return false;
	}
};