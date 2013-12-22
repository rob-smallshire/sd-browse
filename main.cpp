#include <SPI.h>
#include <Ethernet.h>

#include <SdFat.h>

#include "url.hpp"

void renderDirList(EthernetClient& client, const String& path);

const uint8_t SLAVE_SELECT = 53;
const uint8_t SD_CHIP_SELECT = 4;
// file system
SdFat sd;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x44, 0x68 };
IPAddress ip(10, 0, 0, 24);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

template <typename T>
void printkv(const String& key, const T & value) {
    Serial.print(key);
    Serial.print(" = ");
    Serial.println(value);
}

void setup()
{
	Serial.begin(9600);

	pinMode(SLAVE_SELECT, OUTPUT);     // change this to 53 on a mega
	digitalWrite(SLAVE_SELECT, HIGH);  // Disable W5100 Ethernet
	if (!sd.begin(SD_CHIP_SELECT, SPI_HALF_SPEED)) sd.initErrorHalt();

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
	HTTP_DELETE,
};

HttpMethod parseHttpMethod(const String & method_token) {
	if (method_token == "GET") return HTTP_GET;
	if (method_token == "PUT") return HTTP_PUT;
	if (method_token == "POST") return HTTP_POST;
	if (method_token == "DELETE") return HTTP_DELETE;
	return HTTP_UNKNOWN;
}

void readHttpHeaders(EthernetClient & client, long & content_length, String & content_type) {
	while (true) {
		String header_line = readHttpLine(client);
		Serial.print(F("HEADER: "));
		Serial.println(header_line);

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

int readMultipartFormDataHeader(EthernetClient & client, String & boundary, String & disposition, String & content_type) {
    int header_length = 0;
    boundary = readHttpLine(client);
    header_length += boundary.length() + 2;
    //Serial.print(F("BOUNDARY: "));
    //Serial.println(boundary);
    while (true) {
        String header_line = readHttpLine(client);
        header_length += header_line.length() + 2;
        Serial.print(F("MULTIHEADER: "));
        Serial.println(header_line);

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

void httpOk(EthernetClient& client, const String & content_type, long content_length = 0) {
    client.println(F("HTTP/1.1 200 OK"));
    client.print(F("Content-Type: "));
    client.println(content_type);
    if (content_length > 0) {
        client.print(F("Content-Length: "));
        client.println(content_length);
    }
    client.println();
}

void httpOkRedirect(EthernetClient& client, const String & location) {
    client.println(F("HTTP/1.1 200 OK"));
    client.print(F("Location: "));
    client.println(location);
    client.print(F("Content-Length: 0"));
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

String extractValueWithKey(const String & s, const String & key) {
    int index = s.indexOf(' ' + key + '=');
    int start = index + key.length() + 3;
    int end = s.indexOf('"', start);
    return s.substring(start, end);
}

void uploadFile(EthernetClient & client, const String & boundary, const String & path,
                const String & filename, long expected_length) {
    printkv("boundary", boundary);
    printkv("path", path);
    printkv("filename", filename);
    printkv("expected_length", expected_length);

    String full_path = path + filename;

    SdFile new_file;
    if (!new_file.open(full_path.c_str(), O_RDWR | O_CREAT | O_AT_END)) {
       httpInternalServerError(client, "Opening " + filename + " for write failed");
       return;
    }

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
            Serial.print(actual_length);
            Serial.print(" : ");
            Serial.println(expected_length);
            //buffer[num_read] = '\0';
            //for (int i = 0; i < num_read; ++i) {
            //    Serial.print(static_cast<int>(buffer[i]));
            //    Serial.print(' ');
            //}
            //Serial.println();
            new_file.write(buffer, num_read);

            if (actual_length >= expected_length) {
                goto stop;
            }
        }
    }

    stop:

    new_file.close();

    readHttpLine(client); // Swallow newline
    String end_boundary = readHttpLine(client);
    //printkv("end_boundary", end_boundary);
    if (end_boundary != boundary + "--") {
        //Serial.println(F("Missing boundary"));
        new_file.remove();
        httpBadRequest(client, "Missing boundary");
        return;
    }
    renderDirList(client, path);
    return;
}

int readUploadPath(EthernetClient & client, String & path) {
    String boundary;
    String disposition;
    String part_content_type;
    int header_length = readMultipartFormDataHeader(client, boundary, disposition, part_content_type);
    if (header_length == 0) {
        return -1;
    }
    Serial.println(disposition);
    String name = extractValueWithKey(disposition, "name");
    printkv("name", name);
    if (name != "path") {
        return -1;
    }

    // Read two lines, the first of which is the field value, the second of which
    // is the boundary
    path = readHttpLine(client);
    printkv("path", path);
    return header_length + path.length() + 2;
}

int readUploadFilename(EthernetClient & client, String & filename, String & boundary) {
    String disposition;
    String part_content_type;
    int header_length = readMultipartFormDataHeader(client, boundary, disposition, part_content_type);
    if (header_length == 0) {
        return -1;
    }
    Serial.println(disposition);
    String name = extractValueWithKey(disposition, "name");
    if (name != "fileToUpload") {
        return -1;
    }

    filename = extractValueWithKey(disposition, "filename");

    return header_length;
}

void handleFileUpload(EthernetClient & client, const String & content_type, long content_length) {

    String path;
    int part1_length = readUploadPath(client, path);
    if (part1_length < 0) {
        httpBadRequest(client, "Missing path");
    }

    String filename;
    String boundary;
    int part2_length = readUploadFilename(client, filename, boundary);
    if (part2_length < 0) {
        httpBadRequest(client, "Missing filename");
    }

    if (filename.length() > 12) {
        // TODO: Should check for 8.3 compliance
        httpBadRequest(client, "Filename too long.");
        return;
    }

    printkv("content_length", content_length);
    printkv("part1_length", part1_length);
    printkv("part2_length", part2_length);
    printkv("boundary.length()", boundary.length());
    long expected_length = content_length - part1_length - part2_length - (boundary.length() + 6);
    uploadFile(client, boundary, path, filename, expected_length);
}

void hidden_path_field(EthernetClient client, const String & path) {
    client.print(F("<input type=\"hidden\" name=\"path\" value=\""));
    client.print(path);
    client.println(F("\"/>"));
}

void renderBrowseItem(EthernetClient& client, const String& path,
        const String& name, const String & label) {
    client.print(F("<li>"));
    client.print(F("<a href= \"/sd/"));
    client.print(path);
    client.print(name);
    client.print(F("\">"));
    client.print(label);
    client.print(F("</a>"));
    client.println(F("</li>"));
}

void renderDirList(EthernetClient& client, const String& path) {
    SdBaseFile dir;
    bool success =
            path.length() == 0 ?
                    dir.openRoot(sd.vol()) : dir.open(path.c_str(), O_READ);
    if (!success) {
        httpBadRequest(client, "Cannot open directory " + path);
        return;
    }
    dir.rewind();
    httpOk(client, "text/html");
    htmlHeader(client, "Listing - Mistral");
    client.println(F("<body>"));
    client.println(F("<ul>"));
    if (!dir.isRoot()) {
        int slash_index = path.substring(0, path.length() - 1).lastIndexOf('/');
        String parent_path = path.substring(0, slash_index + 1);
        renderBrowseItem(client, parent_path, "", ".. Parent");
    }
    dir_t p;
    while (dir.readDir(&p) > 0) {

        if (p.name[0] == DIR_NAME_FREE)
            break;

        if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.')
            continue;

        if (!DIR_IS_FILE_OR_SUBDIR(&p))
            continue;


        String name;
        for (uint8_t i = 0; i < 11; i++) {
            if (p.name[i] == ' ')
                continue;
            if (i == 8) {
                name += '.';
                //client.print('.');
            }
            name += static_cast<char>(p.name[i]);
            //client.print(static_cast<char>(p.name[i]));
        }
        if (DIR_IS_SUBDIR(&p)) {
            name += '/';
            //client.print('/');
        }

        renderBrowseItem(client, path, name, name);
    }
    client.println(F("</ul>"));
    client.println(
            F("<form name=\"delete\" method=\"post\" action=\"/delete\">"));
    hidden_path_field(client, path);
    client.println(
            F(
                    "<input type=\"text\" name=\"filename\" placeholder=\"File/Directory Name\"/>"));
    client.println(F("<input type=\"submit\" value=\"Delete\" />"));
    client.println(F("</form>"));
    client.println(
            F(
                    "<form id=\"upload\" enctype=\"multipart/form-data\" method=\"post\" action=\"/upload\">"));
    hidden_path_field(client, path);
    client.println(
            F(
                    "<input type=\"file\" name=\"fileToUpload\" id=\"fileToUpload\" />"));
    client.println(F("<input type=\"submit\" value=\"Upload\"/>"));
    client.println(F("</form>"));
    client.println(
            F("<form name=\"mkdir\" method=\"post\" action=\"/mkdir\">"));
    hidden_path_field(client, path);
    client.println(
            F(
                    "<input type=\"text\" name=\"dirname\" placeholder=\"Directory Name\"/>"));
    client.println(F("<input type=\"submit\" value=\"Make Directory\" />"));
    client.println(F("</form>"));
    client.println(F("</body>"));
    client.println(F("</html>"));
}

void handleDirListRequest(EthernetClient & client, const String & path, long content_length) {
    Serial.print(F("DIR: "));
    Serial.println(path);
    skipHttpContent(client, content_length);
    renderDirList(client, path);
}

String contentTypeFromName(const String & path) {
    String lower_path = path;
    lower_path.toLowerCase();
    if (lower_path.endsWith(".htm")) return "text/html";
    if (lower_path.endsWith(".css")) return "text/css";
    if (lower_path.endsWith(".csv")) return "text/csv";
    if (lower_path.endsWith(".xml")) return "text/xml";
    if (lower_path.endsWith(".txt")) return "text/plain";
    if (lower_path.endsWith(".png")) return "image/png";
    if (lower_path.endsWith(".jpg")) return "image/jpeg";
    if (lower_path.endsWith(".gif")) return "image/gif";
    if (lower_path.endsWith(".svg")) return "image/svg+xml";
    if (lower_path.endsWith(".js")) return "application/javascript";
    return "application/octet-stream";
}

void handleFileBrowseRequest(EthernetClient & client, const String & path, long content_length)
{
    Serial.println(F("FILE"));
    Serial.println(path);
    SdFile file;
    if (!file.open(sd.vwd(), path.c_str(), O_READ)) {
       httpNotFound(client, "Could not open " + path);
       return;
    }
    if (!file.isFile()) {
        httpBadRequest(client, "Not a file");
        return;
    }

    String content_type = contentTypeFromName(path);
    Serial.println(content_type);

    uint32_t length = file.fileSize();
    Serial.println(length);

    httpOk(client, content_type, length);

    int16_t c;
    while ((c = file.read()) >= 0) {
        client.print(static_cast<char>(c));
    }

    file.close();
}

void handleFileSystemRequest(EthernetClient & client, const String & url, long content_length) {
    String path = url.substring(4); // len("/sd/")
    if (url.endsWith("/")) {
        handleDirListRequest(client, path, content_length);
    }
    else {
        handleFileBrowseRequest(client, path, content_length);
    }
}

void handleFileDelete(EthernetClient & client,  HttpMethod method, const String & content_type, long content_length) {
    if (method != HTTP_POST) {
        httpMethodNotAllowed(client, "Method not allowed");
    }

    String content;
    readHttpContent(client, content_length, content);

    content = url_decode(content);

    String path_key("path=");
    int pathIndex = content.indexOf(path_key) + path_key.length();

    String filename_key("filename=");
    int filenameIndex = content.indexOf(filename_key, pathIndex) + filename_key.length();
    String path = content.substring(pathIndex, filenameIndex - filename_key.length() - 1);
    String filename = content.substring(filenameIndex);
    printkv("content", content);
    printkv("path", path);
    printkv("filename", filename);

    String full_path = path + filename;

    bool success;
    if (filename.endsWith("/")) {
        success = sd.rmdir(full_path.c_str());
    }
    else {
        success = sd.remove(full_path.c_str());
    }
    if (!success) {
        httpBadRequest(client, "Could not delete " + full_path);
        return;
    }
    renderDirList(client, path);
}

void handleMkDir(EthernetClient & client,  HttpMethod method, const String & content_type, long content_length) {
    if (method != HTTP_POST) {
        httpMethodNotAllowed(client, "Method not allowed");
    }

    String content;
    readHttpContent(client, content_length, content);

    content = url_decode(content);

    String path_key("path=");
    int pathIndex = content.indexOf(path_key) + path_key.length();
    String filename_key("dirname=");
    int filenameIndex = content.indexOf(filename_key, pathIndex) + filename_key.length();
    String path = content.substring(pathIndex, filenameIndex - filename_key.length() - 1);
    String dirname = content.substring(filenameIndex);
    printkv("content", content);
    printkv("path", path);
    printkv("dirname", dirname);

    String full_path = path + dirname;
    bool success = sd.mkdir(full_path.c_str());
    if (!success) {
        httpBadRequest(client, "Could not make directory " + dirname);
        return;
    }
    renderDirList(client, path);
}

void handleRequest(EthernetClient & client, HttpMethod method, const String & url, const String & content_type, long content_length) {
    if (url.startsWith("/sd/")) {
        handleFileSystemRequest(client, url, content_length);
    } else if (url == "/upload") {
        handleFileUpload(client, content_type, content_length);
    } else if (url == "/delete") {
        handleFileDelete(client, method, content_type, content_length);
    } else if (url == "/mkdir") {
        handleMkDir(client, method, content_type, content_length);
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
