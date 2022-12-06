#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "http.h"
#include "jsmn.h"

static int json_eq(char *json, jsmntok_t *tok, const char *s)
{
	if (tok->type == JSMN_STRING
		&& (int)strlen(s) == tok->end - tok->start
		&&  strncmp(json + tok->start, s, tok->end - tok->start) == 0)
			return 0;
	return 1;
}

static char *request_ip()
{
	int req;
	struct http_message http_msg;
	
	if ((req = http_request("https://ifconfig.me/ip")) < 1) {
		fprintf(stderr, "HTTPS connection refused\n");
		exit(1);
	}
	
	memset(&http_msg, 0, sizeof(http_msg));
	
	if (http_response(req, &http_msg) < 1) {
		if (http_msg.header.code && http_msg.header.code != 200) {
			fprintf(stderr,
				"No success response found from the server. Code: %d\n",
				http_msg.header.code);
			exit(1);
		}
	}
	
	if (http_msg.content) {
		int round;
		char *json = http_msg.content;
		jsmn_parser parser;
		jsmntok_t token[300];
		
		jsmn_init(&parser);
		
		round = jsmn_parse(&parser, json, strlen(json), token,
							sizeof(token) / sizeof(token[0]));
		
		if (round < 0) {
			fprintf(stderr, "error: failed to parse response json");
			exit(1);
		}
		return json;
	}
}

int main(int argc, char **argv)
{
	regex_t regex;
	int reg_c;
	int req;
	int color = 0;
	struct http_message http_msg;

	/**
	 * Copied from: https://stackoverflow.com/questions/21554445/regex-for-ip-address-in-c
	*/
	reg_c = regcomp(&regex, 
				"^([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))."
				"([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))."
				"([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))."
				"([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))$",
				REG_EXTENDED);
	
	const char *url = "http://ip-api.com/json/";
	char req_url[200] = {0};

	strcat(req_url, url);
	
	if (argc == 3 && strcmp(argv[1], "-ip") == 0) {
		reg_c = regexec(&regex, argv[2], 0, NULL, 0);

		if (reg_c == REG_NOMATCH) {
			fprintf(stderr, "Invalid IPv4 address\n");
			goto cleanup;
		}
		strcat(req_url, argv[2]);
	} else if (argc == 4 && strcmp(argv[1], "-c") == 0 &&
				strcmp(argv[2], "-ip") == 0) {
		reg_c = regexec(&regex, argv[3], 0, NULL, 0);

		if (reg_c == REG_NOMATCH) {
			fprintf(stderr, "Invalid IPv4 address\n");
			goto cleanup;
		}
		color = 1;
		strcat(req_url, argv[3]);
	} else if (argc == 2 && strcmp(argv[1], "-c") == 0) {
		color = 1;
		strcat(req_url, request_ip());
	} else if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		fprintf(stdout, "whatip\n\n"
						"Usage:\nwhatip -ip [IPv4 address]\n"
						"whatip -c -ip [IPv4 address]\n");
		exit(0);
	} else {
		strcat(req_url, request_ip());
	}

	if ((req = http_request(req_url)) < 1) {
		fprintf(stderr, "HTTP connection failed");
		exit(1);
	}
	
	memset(&http_msg, 0, sizeof(http_msg));
	
	if (http_response(req, &http_msg) < 1) {
		if (http_msg.header.code && http_msg.header.code != 200) {
			fprintf(stderr,
				"No success response found from the server. Code: %d\n",
				http_msg.header.code);
			goto cleanup;
		}
	}
	
	if (http_msg.content) {
		int round, i;
		char *json = http_msg.content;
		jsmn_parser parser;
		jsmntok_t token[400];
		
		jsmn_init(&parser);
		
		round = jsmn_parse(&parser, json, strlen(json), token,
							sizeof(token) / sizeof(token[0]));
		
		if (round < 0) {
			fprintf(stderr, "Failed to parse response json");
			goto cleanup;
		}
		
		if (round < 1 || token[0].type != JSMN_OBJECT) {
			fprintf(stderr,
				"We expect server response as an object");
			goto cleanup;
		}
		
		for (i = 1; i < round; i++) {
			if (json_eq(json, &token[i], "country") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mCountry: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Country: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "countryCode") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mCode: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Code: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "region") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mRCode: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ RCode: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "regionName") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mRegion: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Region: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "city") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mCity: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ City: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}

			if (json_eq(json, &token[i], "zip") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mZip: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Zip: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "lat") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mLatitude: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Latitude: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "lon") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mLongitude: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Longitude: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "timezone") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mTimezone: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ Timezone: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "isp") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mISP: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ ISP: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "org") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mOrganization: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
						"‣ Organization: %.*s\n",
						token[i + 1].end - token[i + 1].start,
						json + token[i + 1].start);
			}
			
			if (json_eq(json, &token[i], "as") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mAS: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ AS: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}

			if (json_eq(json, &token[i], "query") == 0) {
				if ((token[i + 1].end - token[i + 1].start) != '\0')
					fprintf(stdout, color ?
							"\033[0;97m‣\033[0m \033[0;93mIP: \033[0m"
							"\033[0;92m%.*s\033[0m\n" :
							"‣ IP: %.*s\n",
							token[i + 1].end - token[i + 1].start,
							json + token[i + 1].start);
			}
		}

		goto cleanup;
	}

cleanup:
	regfree(&regex);
	exit(0);
}
