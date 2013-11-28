#include <SPI.h>
#include <Ethernet.h>

#include "sdcard.hpp"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x44, 0x68 };
IPAddress ip(10, 0, 0, 24);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup()
{
	Serial.begin(9600);

	sd::initialize();

	Ethernet.begin(mac, ip);
	Serial.println(F("Beginning server..."));
	server.begin();
}

#define HTTP_BUFFER_SIZE 255


/**
 *  Read a line from the EthernetClient.
 */
String readHttpLine(EthernetClient & client) {
	char buffer[HTTP_BUFFER_SIZE + 1];
	int index = 0;
	while (client.connected()) {
		if (client.available()) {
			int b = client.read();

			if (b == -1) { // no data
				break;
			}

			//Serial.println(b);
			//Serial.flush();

			char c = static_cast<char>(b);

			if (c == '\r') {   // carriage-return
				b = client.read(); // line-feed
				//Serial.println(b);
				//Serial.flush();
				break;
			}

	        buffer[index] = c;
	        ++index;

            if (index == HTTP_BUFFER_SIZE) {
                break;
            }
	    }
	}
	buffer[index] = '\0';
	//Serial.println(index);
	return String(buffer);
}

enum HttpMethod {
	HTTP_UNKNOWN,
	HTTP_GET,
	HTTP_PUT,
	HTTP_POST,
};

HttpMethod parseHttpMethod(const String & method_token) {
	if (method_token == "GET") return HTTP_GET;
	if (method_token == "PUT") return HTTP_PUT;
	if (method_token == "POST") return HTTP_POST;
	return HTTP_UNKNOWN;
}

void readHttpHeaders(EthernetClient & client, long & content_length, String & content_type) {
	while (true) {
		String header_line = readHttpLine(client);
		//Serial.print(F("HEADER: "));
		//Serial.println(header_line);

		if (header_line.startsWith("Content-Length:")) {
			content_length = header_line.substring(16).toInt();
			//Serial.print(F("Content length = "));
			//Serial.println(content_length);
		}
		else if (header_line.startsWith("Content-Type:")) {
			content_type = header_line.substring(14);
			//Serial.print(F("Content type = "));
			//Serial.println(content_type);
		}

		if (header_line.length() == 0) {
			break;
		}
	}
}

int readMultipartFormDataHeaders(EthernetClient & client, String & boundary, String & disposition, String & content_type) {
    int header_length = 0;
    boundary = readHttpLine(client);
    header_length += boundary.length() + 2;
    //Serial.print(F("BOUNDARY: "));
    //Serial.println(boundary);
    while (true) {
        String header_line = readHttpLine(client);
        header_length += header_line.length() + 2;
        //Serial.print(F("MULTIHEADER: "));
        //Serial.println(header_line);

        if (header_line.startsWith("Content-Disposition:")) {
            disposition = header_line.substring(21);
            //Serial.print(F("Disposition = "));
            //Serial.println(disposition);
        }
        else if (header_line.startsWith("Content-Type:")) {
            content_type = header_line.substring(14);
            //Serial.print(F("Content type = "));
            //Serial.println(content_type);
            //Serial.flush();
        }
        Serial.println(header_line.length());
        if (header_line.length() == 0) {
            //Serial.println(F("End header!"));
            break;
        }
    }
    return header_length;
}

void skipHttpContent(EthernetClient& client, long content_length) {
    for (long i = 0; i < content_length; ++i) {
        int b = client.read();
        if (b == -1) {
            break;
        }
     }
}

void readHttpContent(EthernetClient& client, long content_length, String& content) {
	for (long i = 0; i < content_length; ++i) {
		int b = client.read();
		if (b == -1) {
			break;
		}
		char c = static_cast<char>(b);
		content += c;
	}

	//Serial.print(F("CONTENT: "));
	//Serial.println(content);
}

/**
 * Simple HTTP parser for GET and PUT requests.
 *
 * Args:
 *     client: An EthernetClient
 *     url: A String out parameter which will contain the URL
 *     content_length: A int out parameter which will contain the content-length or -1
 *
 * Returns:
 *     GET or PUT
 */
HttpMethod readHttpRequest(EthernetClient & client, String & url, String & content_type, long& content_length) {
    String request = readHttpLine(client);
    int end_method = request.indexOf(' ');
	String method_token = request.substring(0, end_method);
	Serial.println(method_token);
	HttpMethod method = parseHttpMethod(method_token);

	int end_url = request.indexOf(' ', end_method + 1);
	url = request.substring(end_method + 1, end_url);
	Serial.println(url);

	content_length = -1;
	readHttpHeaders(client, content_length, content_type);



	// TODO: Add a check for the Host header - important for compliance with HTTP/1.1

	//client.flush();

	return method;
}


void httpBadRequest(EthernetClient& client, const String & content) {
	client.println(F("HTTP/1.1 400 Bad Request"));
	client.println(F("Content-Type: text/plain"));
	client.print(F("Content-Length: "));
	client.println(content.length());
	client.println();
	client.print(content);
}

void httpMethodNotAllowed(EthernetClient& client, const String & content) {
	client.println(F("HTTP/1.1 405 Method Not Allowed"));
	client.println(F("Content-Type: text/plain"));
	client.print(F("Content-Length: "));
	client.println(content.length());
	client.println();
	client.print(content);
}

void httpNotFound(EthernetClient& client, const String & content) {
	client.println(F("HTTP/1.1 404 Not Found"));
	client.println(F("Content-Type: text/plain"));
	client.print(F("Content-Length: "));
	client.println(content.length());
	client.println();
	client.print(content);
}

void httpGone(EthernetClient& client) {
	client.println(F("HTTP/1.1 410 Gone"));
	client.print(F("Content-Length: 0"));
	client.println();
}

void httpInternalServerError(EthernetClient& client, const String & content) {
    client.println(F("HTTP/1.1 500 Internal Server Error"));
    client.println(F("Content-Type: text/plain"));
    client.print(F("Content-Length: "));
    client.println(content.length());
    client.println();
    client.print(content);
}

void httpServiceUnavailable(EthernetClient& client, const String & content) {
	client.println(F("HTTP/1.1 503 Service Unavailable"));
	client.println(F("Content-Type: text/plain"));
	client.print(F("Content-Length: "));
	client.println(content.length());
	client.println();
	client.print(content);
}

template <typename T>
String makeString(T value) {
	return String(value);
}

#define DECIMAL_PLACES 2

template <>
String makeString(float value) {
    char buffer[33];
    char* s = dtostrf(value, DECIMAL_PLACES + 2, DECIMAL_PLACES, buffer);
    return String(s);
}

template <>
String makeString(String value) {
    return value;
}

template <typename T>
void httpOkScalar(EthernetClient& client, T scalar) {
	String response_content = makeString(scalar);
	client.println(F("HTTP/1.1 200 OK"));
	client.println(F("Content-Type: text/plain"));
	client.print(F("Content-Length: "));
	client.println(response_content.length());
	client.println();
	client.println(response_content);
}

void httpOk(EthernetClient& client, const String & content_type) {
    client.println(F("HTTP/1.1 200 OK"));
    client.print(F("Content-Type: "));
    client.println(content_type);
    client.println();
}

void htmlHeader(EthernetClient& client, const String & title) {
    client.println(F("<!DOCTYPE html>"));
    client.println(F("<html lang=\"en\">"));
    client.println(F("<head>"));
    client.println(F("<meta charset=\"utf-8\">"));
    client.print(F("<title>"));
    client.print(title);
    client.println(F("</title>"));
    client.println(F("</head>"));
}

void handleUploadRequest(EthernetClient & client, long content_length) {
    skipHttpContent(client, content_length);

    httpOk(client, "text/html");
    htmlHeader(client, "Upload - Mistral");
    client.println(F("<body>"));
    client.println(F("<body>"));
    client.println(F("<form id=\"form1\" enctype=\"multipart/form-data\" method=\"post\" action=\"submit\">"));
    client.println(F("<label for=\"fileToUpload\">Select a File to Upload</label><br />"));
    client.println(F("<input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\" />"));
    client.println(F("<input type=\"submit\" />"));
    client.println(F("</form>"));
    client.println(F("</body>"));
    client.println(F("</html>"));
}

void handleFileUpload(EthernetClient & client, const String & content_type, long content_length) {
    String boundary;
    String disposition;
    String part_content_type;
    int header_length = readMultipartFormDataHeaders(client, boundary, disposition, part_content_type);
    Serial.println(disposition);

    String filename_key = String("filename=");
    int filename_index = disposition.indexOf("filename=");
    int filename_start = filename_index + filename_key.length() + 1;
    int filename_end = disposition.indexOf('"', filename_start);
    String filename = disposition.substring(filename_start, filename_end);

    long expected_length = content_length - header_length - (boundary.length() + 4);

    //SdFile myFile;
    //if (!myFile.open(filename, O_READ)) {
    //   httpInternalServerError(client, "Opening " + filename + " for write failed");
    //}

    // TODO: Watchdog timer
    uint8_t buffer[HTTP_BUFFER_SIZE + 1];
    long actual_length = 0;
    while (client.connected()) {
        while (int available = client.available()) {
            long num_remaining = expected_length - actual_length;
            int num_to_read = min(min(available, HTTP_BUFFER_SIZE), num_remaining);
            int num_read = client.read(buffer, num_to_read);
            if (num_read < 0) {
                break;
            }
            actual_length += num_read;
            //Serial.print(actual_length);
            //Serial.print(" : ");
            //Serial.println(expected_length);
            //buffer[num_read] = '\0';
            //Serial.print(reinterpret_cast<char*>(buffer));

            if (actual_length >= expected_length) {
                goto stop;
            }
        }
    }

    stop:
    String end_boundary = readHttpLine(client);
    if (end_boundary == boundary + "--") {
        Serial.println(F("Found boundary   "));
        String message(actual_length);
        message += " bytes uploaded.";
        httpOkScalar(client, message);
    }
    else {
        Serial.println(F("Missing boundary"));
        httpBadRequest(client, "Missing boundary");
    }
}

void handleHomeRequest(EthernetClient & client, long content_length) {
    skipHttpContent(client, content_length);

    httpOk(client, "text/html");
    htmlHeader(client, "Listing - Mistral");
	client.println(F("<body>"));
    uint8_t flags = 0;
    dir_t p;
    sd::root().rewind();
    while (sd::root().readDir(&p) > 0) {
        if (p.name[0] == DIR_NAME_FREE) break;

        if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;

        if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

        for (uint8_t i = 0; i < 11; i++) {
          if (p.name[i] == ' ') continue;
          if (i == 8) {
            client.print('.');
          }
          client.print(static_cast<char>(p.name[i]));
        }
        if (DIR_IS_SUBDIR(&p)) {
          client.print('/');
        }

        // print modify date/time if requested
        if (flags & LS_DATE) {
           sd::root().printFatDate(p.lastWriteDate);
           client.print(' ');
           sd::root().printFatTime(p.lastWriteTime);
        }
        // print size if requested
        if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE)) {
          client.print(' ');
          client.print(p.fileSize);
        }
        client.println("<br>");
    }
    client.println(F("</body>"));
    client.println(F("</html>"));
}

void handleRequest(EthernetClient & client, HttpMethod method, const String & url, const String & content_type, long content_length) {
    if (url == "/root") {
		handleHomeRequest(client, content_length);
    } else if (url == "/upload") {
        handleUploadRequest(client, content_length);
    } else if (url == "/submit") {
        handleFileUpload(client, content_type, content_length);
	} else if (url == "/favicon.ico") {
		httpGone(client);
	} else {
		httpNotFound(client, "No resource at this URL");
	}
}

void loop()
{
    // listen for incoming clients
    EthernetClient client = server.available();
    if (client) {
	    String url;
	    String content_type;
	    long content_length;
	    HttpMethod method = readHttpRequest(client, /*out*/ url, /*out*/ content_type, /*out*/ content_length);
		handleRequest(client, method, url, content_type, content_length);
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
}
