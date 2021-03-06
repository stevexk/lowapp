/**
 * @file configuration.c
 * @brief Node management for LoWAPP simulation
 *
 * @date August 9, 2016
 * @author Nathan Olff
 */

#include "configuration.h"
#include "lowapp_utils_conversion.h"
#include "lowapp_log.h"
#include <uuid/uuid.h>

extern char configFile[];
extern char nodeSubdir[];

/* Include string literal for configuration values */
extern const uint8_t strDeviceId[];
extern const uint8_t strGroupId[];
extern const uint8_t strGwMask[];
extern const uint8_t strEncKey[];
extern const uint8_t strRchanId[];
extern const uint8_t strRsf[];
extern const uint8_t strPreambleTime[];

/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simu_config
 * @{
 */

/** Configuration variables are stored in this structure */
ConfigNode_t myConfig;



/**
 * @brief Get a configuration value using its key
 *
 * The variable's value is stored as a string in
 * the value buffer sent as parameter.
 *
 * @param key Key of the expected configuration variable (as a string)
 * @param value Buffer in which to store the value (as a string)
 */
int8_t get_config(const uint8_t* key, uint8_t* value) {
	const char* keyChar = (const char*) key;
	if(strcmp(keyChar, (const char*)strGwMask) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, (uint8_t*)&(myConfig.gwMask), 4, true);
	}
	else if(strcmp(keyChar, (const char*)strDeviceId) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, &(myConfig.deviceId), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strGroupId) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, (uint8_t*)&(myConfig.groupId), 2, true);
	}
	else if(strcmp(keyChar, (const char*)strRchanId) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, &(myConfig.rchanId), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strRsf) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, &(myConfig.rsf), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strPreambleTime) == 0) {
		return FillBuffer16_t((uint8_t*)value, 0, &(myConfig.preambleTime), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strEncKey) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, myConfig.encKey, 16, true);
	}
	else {
		return -1;
	}
	return 0;
}


/**
 * @brief Set a configuration variable using its key
 *
 * @param key Key of the configuration variable to set (as a string)
 * @param val New value of the configuration variable
 * @retval 0 If the key was found and the value set
 * @retval -1 If the key was not found
 */
int8_t set_config(const uint8_t* key, const uint8_t* val) {
	const char* keyChar = (const char*) key;
	if(strcmp(keyChar, (const char*)strGwMask) == 0) {
		AsciiHexStringConversionBI8_t((uint8_t*)&(myConfig.gwMask), val, 8);
	}
	else if(strcmp(keyChar, (const char*)strDeviceId) == 0) {
		AsciiHexStringConversionBI8_t(&(myConfig.deviceId), val, 2);
	}
	else if(strcmp(keyChar, (const char*)strGroupId) == 0) {
		AsciiHexStringConversionBI8_t((uint8_t*)&(myConfig.groupId), val, 4);
	}
	else if(strcmp(keyChar, (const char*)strRchanId) == 0) {
		AsciiHexConversionOneValueBI8_t((uint8_t*)&(myConfig.rchanId), val);
	}
	else if(strcmp(keyChar, (const char*)strRsf) == 0) {
		AsciiHexConversionOneValueBI8_t((uint8_t*)&(myConfig.rsf), val);
	}
	else if(strcmp(keyChar, (const char*)strPreambleTime) == 0) {
		myConfig.preambleTime = AsciiDecStringConversion_t(val, strlen((char*)val));
	}
	else if(strcmp(keyChar, (const char*)strEncKey) == 0) {
		AsciiHexStringConversionBI8_t(myConfig.encKey, (const uint8_t*)val, 32);
	}
	else {
		return -1;
	}
	return 0;
}

/** @} */
/** @} */

/**
 * @brief Get a uuid to store device specific informations
 *
 * The uuid can be either generated by the application or
 * sent to it as an argument
 *
 * @param argc Number of parameters sent to the main function
 * @param argv Array of parameters sent to the main function
 *
 * @retval -1 If the argument was not a valid uuid
 * @retval 1 If the argument was a valid uuid
 * @retval 0 If a new uuid was generated
 */
int8_t get_uuid(int argc, char* argv[]) {
	uuid_t uuid;
	char uuid_str[37];
	int ret;
	/*
	 * If a parameter was sent to the main function, check that it is
	 * a uuid and parse it
	 */
	if(argc == 2) {

	}
	else {
		/* If no parameter was sent, we generate a uuid */
		uuid_generate(uuid);
		uuid_unparse(uuid, uuid_str);
		ret = 0;
	}
	/* Store path of the config file */
	sprintf(configFile, "Nodes/%s", uuid_str);
	return ret;
}

/**
 * @brief Parse a line of the configuration file into a
 * key and a value
 *
 * @param line Line to parse
 * @retval 0 If a key was found and the value was set
 * @retval -1 If the kay was not found or the value not set
 */
int8_t parse_line(char* line) {
	char* ptr_key;
	char* ptr_val;
	/* Split line around ':' */
	ptr_key = strtok(line, ":");
	if(ptr_key != NULL) {
		ptr_val = strtok(NULL, ":");
		/* Set configuration value according to the value from the configuration file */
		return set_config((uint8_t*)ptr_key, (uint8_t*)ptr_val);
	}
	return -1;
}


/**
 * @brief Initialise a node by analyzing program's arguments
 *
 * @param args Program's arguments for argp processing
 * @retval 0 If a uuid was found for the device
 * @retval -1 If no uuid was found / generated
 */
int8_t node_init(struct arguments *args) {
	uuid_t uuid;
	memset(configFile, 0, 100);
	/* If a config file path was given as argument, check the file exists */
	if(args->config != NULL) {
		/* Check if the argument was the path to the file */
		if(file_exists(args->config)) {
			strcpy(configFile, args->config);
			return 0;
		}
		else {
			/*
			 * If config is not a relative path to the working directory, check if it is
			 * in the directory referred to by the --directory argument
			 */
			if(args->directory != NULL) {
				strcpy(configFile, args->directory);
				strcat(configFile, args->config);
				if(file_exists(configFile)) {
					return 0;
				}
				else {
					LOG(LOG_FATAL, "The config file (%s) does not exists", configFile);
					return -1;
				}
			}
			else {
				LOG(LOG_FATAL, "The config file (%s) does not exists", args->config);
				return -1;
			}
		}
	}
	/* If a uuid and a directory was given, check if the file is there */
	else if(args->uuid != NULL && args->directory != NULL) {
		/* If not, check if it is a uuid */
	   if(uuid_parse(args->uuid, uuid) != 0){
		   LOG(LOG_FATAL, "The UUID passed as parameter is not valid");
		   return -1;
	   }
	   else {
		strcpy(configFile, args->directory);
		strcat(configFile, nodeSubdir);
		strcat(configFile, args->uuid);
		if(file_exists(configFile)) {
			return 0;
		}
		else {
			LOG(LOG_FATAL, "The config file (%s) does not exists", configFile);
			return -1;
		}
		return 0;
	   }
	}
	else {
		LOG(LOG_FATAL, "Not enough parameters were sent to the program.\nFor correct usage, see --help option");
		return -1;
	}
}
