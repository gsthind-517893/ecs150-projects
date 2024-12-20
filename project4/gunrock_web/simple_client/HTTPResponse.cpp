#include "HTTPResponse.h"

#include <iostream>
#include <string>

#include <assert.h>
#include <errno.h>

#include <sstream>

using namespace std;

HTTPResponse::HTTPResponse(MySocket *sock) {
    m_sock = sock;
    m_status_code = 0;
}


string HTTPResponse::readResponse() {
  string full_response;

  while (true) {
    string response;
    try {
      response = m_sock->read();
    } catch (...) {
      break;
    }

    full_response += response;
  }

  size_t delimiter = full_response.find("\r\n\r\n");
  if (delimiter == string::npos) {
    return "";
  }

  m_body = full_response.substr(delimiter+4);
  string header_string = full_response.substr(0, delimiter);
  stringstream header_stream(header_string);

  string line;
  while (getline(header_stream, line)) {
    if (line.find("HTTP/1.1 ") == 0) {
      stringstream header_line(line);
      string http;
      header_line >> http >> m_status_code >> m_status_message;
    }
  }
  
  return m_body;
}
