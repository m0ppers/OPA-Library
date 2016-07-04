﻿/**
	fredslab.net : OPA Shield library for Arduino

	The MIT License (MIT)

	Source code copyright (c) 2013-2016 Frédéric Meslin / Thomas Hopper
	Email: fredericmeslin@hotmail.com
	Website: www.fredslab.net
	Twitter: @marzacdev

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "OPA.h"
#include "Arduino.h"

/*****************************************************************************/
#define OPA_CS1_STATE(a) (a & 0x01)
#define OPA_CS2_STATE(a) ((a >> 1) & 0x01)

/*****************************************************************************/
OPA::OPA() :
	address(OPA_ADDRESS_0),
	error(OPA_ERROR_NONE)
{
	initPins();
}

OPA::OPA(OPA_ADDRESSES address) :
	address(address),
	error(OPA_ERROR_NONE)
{
	initPins();
}

/*****************************************************************************/
void OPA::initPins()
{
	pinMode(OPA_TX_PIN, INPUT);
	pinMode(OPA_RX_PIN, INPUT);
	pinMode(OPA_CS1_PIN, OUTPUT);
	pinMode(OPA_CS2_PIN, OUTPUT);
	pinMode(OPA_SWAP_PIN, OUTPUT);
	pinMode(OPA_RESET_PIN, OUTPUT);

	digitalWrite(OPA_CS1_PIN, 1);
	digitalWrite(OPA_CS2_PIN, 1);
	digitalWrite(OPA_SWAP_PIN, 0);
	digitalWrite(OPA_RESET_PIN, 0);
}

/*****************************************************************************/
void OPA::enable()
{
	digitalWrite(OPA_CS1_PIN, OPA_CS1_STATE(address));
	digitalWrite(OPA_CS2_PIN, OPA_CS2_STATE(address));
	Serial.begin(OPA_BAUDRATE);
	Serial.setTimeout(OPA_SERIAL_TIMEOUT);
	clearErrors();
}

void OPA::disable()
{
	Serial.end();
	digitalWrite(OPA_CS1_PIN, 1);
	digitalWrite(OPA_CS2_PIN, 1);
}

/*****************************************************************************/
void OPA::reset()
{
	digitalWrite(OPA_RESET_PIN, 1);
	delay(2);
	digitalWrite(OPA_RESET_PIN, 0);
	delay(2);
	error = OPA_ERROR_NONE;
}

/*****************************************************************************/
char * OPA::readVersion()
{
	static char version[24];
	char buffer[1];
	buffer[0] = OPA_CODE_VERSION;
	Serial.write(buffer, 1);
	if (Serial.readBytes(version, 24) == 24)
		return version;
	error = OPA_ERROR_TIMEOUT;
	return 0;
}

/*****************************************************************************/
void OPA::noteOn(OPA_PROGRAMS program, uint8_t note, uint8_t fraction, uint8_t nuance)
{
	char buffer[5];
	buffer[0] = OPA_CODE_NOTEON;
	buffer[1] = program;
	buffer[2] = note;
	buffer[3] = fraction;
	buffer[4] = nuance;
	Serial.write(buffer, 5);
}

void OPA::noteOff(OPA_PROGRAMS program, uint8_t note, uint8_t fraction, uint8_t nuance)
{
	char buffer[5];
	buffer[0] = OPA_CODE_NOTEOFF;
	buffer[1] = program;
	buffer[2] = note;
	buffer[3] = fraction;
	buffer[4] = nuance;
	Serial.write(buffer, 5);
}

void OPA::allNotesOff(OPA_PROGRAMS program)
{
	char buffer[2];
	buffer[0] = OPA_CODE_ALLNOTESOFF;
	buffer[1] = program;
	Serial.write(buffer, 2);
}

void OPA::allSoundsOff()
{
	char buffer[2];
	buffer[0] = OPA_CODE_ALLSOUNDSOFF;
	Serial.write(buffer, 1);
}

/*****************************************************************************/
void OPA::pitchBend(OPA_PROGRAMS program, int8_t coarse, int8_t fine)
{
	char buffer[4];
	buffer[0] = OPA_CODE_PITCHBEND;
	buffer[1] = program;
	buffer[2] = coarse;
	buffer[3] = fine;
	Serial.write(buffer, 4);
}

/*****************************************************************************/
void OPA::setMemoryProtection(bool protection)
{
	int flags = readGlobalParam(OPA_GLOBAL_FLAGS);
	flags &= ~OPA_GLOBAL_PROTECT;
	if (protection) flags |= OPA_GLOBAL_PROTECT;
	writeGlobalParam(OPA_GLOBAL_FLAGS, flags);
}

/*****************************************************************************/
void OPA::writeOperatorParam(OPA_PROGRAMS program, OPA_OPERATORS op, OPA_OP_PARAMETERS param, uint8_t value)
{
	uint8_t p = op * OPA_OP_PARAMS_NB + OPA_PROGS_PARAMS_NB + param;
	writeFMParam(program, p, value);
}

uint8_t OPA::readOperatorParam(OPA_PROGRAMS program, OPA_OPERATORS op, OPA_OP_PARAMETERS param)
{
	uint8_t p = op * OPA_OP_PARAMS_NB + OPA_PROGS_PARAMS_NB + param;
	return readFMParam(program, p);
}

/*****************************************************************************/
void OPA::writeGlobalParam(OPA_GLOBAL_PARAMETERS param, uint8_t value)
{
/** Send a parameter */
	char buffer[3];
	buffer[0] = OPA_CODE_GLOBALSPARAMWRITE;
	buffer[1] = param;
	buffer[2] = value;
	Serial.write(buffer, 6);
}

void OPA::writeFMParam(OPA_PROGRAMS program, uint8_t param, uint8_t value)
{
/** Send a parameter */
	char buffer[4];
	buffer[0] = OPA_CODE_FMPARAMWRITE;
	buffer[1] = program;
	buffer[2] = param;
	buffer[3] = value;
	Serial.write(buffer, 4);
}

uint8_t OPA::writeKitParam(int sample, OPA_SAMPLE_PARAMETERS param, uint8_t value)
{
/** Send a parameter */
	char buffer[4];
	buffer[0] = OPA_CODE_KITPARAMWRITE;
	buffer[1] = sample;
	buffer[2] = param;
	buffer[3] = value;
	Serial.write(buffer, 4);
}

uint8_t OPA::readGlobalParam(OPA_GLOBAL_PARAMETERS param)
{
/** Send a parameter request */
	char buffer[3];
	buffer[0] = OPA_CODE_GLOBALSPARAMREAD;
	buffer[1] = param;
	buffer[2] = 0;
	Serial.write(buffer, 3);

/** Check the reply */
	if (Serial.readBytes(buffer, 3) == 3) {
		if (buffer[0] == OPA_CODE_GLOBALSPARAMWRITE)
			return buffer[2];
		error = OPA_ERROR_BADREPLY;
		return 0;
	}
	error = OPA_ERROR_TIMEOUT;
	return 0;
}

uint8_t OPA::readFMParam(OPA_PROGRAMS program, uint8_t param)
{
/** Send a parameter request */
	char buffer[4];
	buffer[0] = OPA_CODE_FMPARAMREAD;
	buffer[1] = program;
	buffer[2] = param;
	buffer[3] = 0;
	Serial.write(buffer, 4);

/** Check the reply */
	if (Serial.readBytes(buffer, 4) == 4) {
		if (buffer[0] == OPA_CODE_FMPARAMWRITE)
			return buffer[3];
		error = OPA_ERROR_BADREPLY;
		return 0;
	}
	error = OPA_ERROR_TIMEOUT;
	return 0;
}

uint8_t OPA::readKitParam(int sample, OPA_SAMPLE_PARAMETERS param)
{
/** Send a parameter request */
	char buffer[4];
	buffer[0] = OPA_CODE_KITPARAMREAD;
	buffer[1] = sample;
	buffer[2] = param;
	buffer[3] = 0;
	Serial.write(buffer, 4);

/** Check the reply */
	if (Serial.readBytes(buffer, 4) == 4) {
		if (buffer[0] == OPA_CODE_KITPARAMWRITE)
			return buffer[3];
		error = OPA_ERROR_BADREPLY;
		return 0;
	}
	error = OPA_ERROR_TIMEOUT;
	return 0;
}


/*****************************************************************************/
void OPA::writeProgram(OPA_PROGRAMS program, OpaProgram &programData)
{

}

void OPA::readProgram(OPA_PROGRAMS program, OpaProgram &programData)
{
}


/*****************************************************************************/
void OPA::writeKit(OpaKit &kitData)
{
}

void OPA::readKit(OpaKit &kitData)
{
}

/*****************************************************************************/
void OPA::storeInternal(OPA_PROGRAMS program, uint8_t slot)
{
	char buffer[3];
	if (slot >= OPA_MAX_SLOTS) {
		error = OPA_ERROR_BADPARAMETER;
		return;
	}
	buffer[0] = OPA_CODE_INTERNALSTORE;
	buffer[1] = program;
	buffer[2] = slot;
	Serial.write(buffer, 3);
}

void OPA::loadInternal(OPA_PROGRAMS program, uint8_t slot)
{
	char buffer[3];
	if (slot >= OPA_MAX_SLOTS) {
		error = OPA_ERROR_BADPARAMETER;
		return;
	}
	buffer[0] = OPA_CODE_INTERNALLOAD;
	buffer[1] = program;
	buffer[2] = slot;
	Serial.write(buffer, 3);
}

/*****************************************************************************/
void OPA::writeInternal(uint8_t slot, OpaProgram &programData)
{
}

void OPA::readInternal(uint8_t slot, OpaProgram &programData)
{
}


