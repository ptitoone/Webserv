#include "Request.hpp"

static str_t&	_clean(str_t& str) {
	while (str[0] == ' ')
		str.erase(0, 1);
	if (*str.rbegin() == '\r')
		str.erase(str.end() - 1);
	return str;
}

static bool	_bad_rline(const strs_t& rline) {
	str_t::const_iterator	it;

	if (rline.size() != 3)
		return true;
	for (it = rline[0].begin(); it != rline[0].end(); ++it) {
		if (!isupper(*it))
			return true;
	}
	if (rline[1][0] != '/')
		return true;
	return false;
}

static void url_decode(str_t& url) {
	std::string	value;
	size_t		pos = 0;

	while ((pos = url.find("%", pos)) != std::string::npos) {
		value = url.substr(pos+1, 2);
		value = std::strtol(value.c_str(), 0, 16);
		url.erase(pos, 3);
		url.insert(pos, 1, *value.c_str());
		++pos;
	}
}

Request::Request()
{ std::cout << "Create an http request" << std::endl; }

Request::~Request()
{ std::cout << "Destroy Request" << std::endl; }

int	Request::_rline_checker() {
	if (_bad_rline(_start))
		return RESPONSE | ERROR | ERR_400;
	if (_start[2].substr(0, 5) != "HTTP/") {
		if (_start[0] != "GET" || _start[0] != "POST" || _start[0] != "DELETE")
			return RESPONSE | ERROR | ERR_400;
		return RESPONSE | ERROR | ERR_404;
	}
	else if (_start[2].substr(5) != "1.1")
		return RESPONSE | ERROR | ERR_505;
	url_decode(_start[1]);
	return HEADER;
}

int	Request::_headers_checker() {
	if (_headers.find("host") == _headers.end())
		return RESPONSE | ERROR | ERR_400;
	else if (_headers.find("transfer-encoding") != _headers.end()) {
		if (_headers.find("transfer-encoding")->second != "chunked")
			return RESPONSE | ERROR | ERR_501;
		return BODY;
	}
	else if (_headers.find("content-length") != _headers.end())
		return BODY;
	return RESPONSE;
}

/*
Aim: extract request line
skip first empty lines
perform error check when LF reached
save the peer request line or throw an error
*/
int	Request::process_rl(str_t& raw_data) {
	std::stringstream	tmp;
	size_t	limit;
	str_t	elem;

	while (raw_data[0] == LF)
		raw_data.erase(0, 1);
	limit = raw_data.find(LF);
	if (limit != std::string::npos) { // complete request line
		tmp.str(raw_data.substr(0, limit));
		raw_data.erase(0, limit + 1);
		while (tmp >> elem)
			_start.push_back(elem);
		return _rline_checker();
	}
	return RQLINE;
}

int	Request::process_head(str_t& raw_data) {
	str_t	hline, value, key;
	size_t	limit, found;

	do {
		limit = raw_data.find(LF);
		if (limit == 0 || (limit == 1 && raw_data[0] == CR)) { // end of line headers
			raw_data.erase(0, limit + 1);
			return _headers_checker();
		}
		else if (limit != std::string::npos) {
			hline = raw_data.substr(0, limit);
			raw_data.erase(0, limit + 1);
			found = hline.find(":");
			if (found != std::string::npos) { // good header line
				key = hline.substr(0, found);
				value = hline.substr(found + 1);
				_headers.insert(std::make_pair(_tolower(key), _clean(value)));
			}
		}
	} while (!raw_data.empty() && limit != std::string::npos);
	return HEADER;
}

int	Request::process_body(str_t& raw_data) {
	fields_t::const_iterator	it;
	size_t	len;

	it = _headers.find("content-length");
	if (it != _headers.end()) {
		len = _atoi(it->second, 10);
		_body.append(raw_data);
		raw_data.clear();
		if (_body.size() < len)
			return BODY;
		return RESPONSE;
	}
	return process_chunk(raw_data);
}

int	Request::process_chunk(str_t& raw_data) {
	static bool		read_size = true;
	static size_t	length = 0;
	size_t chunk_size;
	size_t limit;

	do {
		limit = raw_data.find(LF);
		if (limit != std::string::npos) {
			if (read_size == true) {
				if (!(chunk_size = _atoi(raw_data.substr(0, limit), 16)))
					return RESPONSE;
				raw_data.erase(0, limit + 1);
				length += chunk_size;
				read_size = false;
			}
			else {
				_body.append(raw_data.substr(0, chunk_size));
				raw_data.erase(0, limit + 1);
				read_size = true;
			}
		}
	} while (!raw_data.empty() && limit != std::string::npos);
	return BODY;
}
