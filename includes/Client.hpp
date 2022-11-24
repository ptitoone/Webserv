#pragma once

# ifndef CLIENT_HPP
# define CLIENT_HPP

# include "Request.hpp"
# include "Response.hpp"

class Response;

/*
** class Client:
** manage incoming connection
** save servers which listens on this connection
** format request
** format response
** save current state
*/
class	Client {
	public:
		Client();
		~Client();

		const Server*	get_server() const;
		const Request*	get_request() const;

		void	set_server(const Server* server);

		void	reset();
		void	process_request(int& state);
		void	process_response(int socket, int& state);
		void	set_headers(const str_t& path, int code);

		str_t	raw_data;

	private:
		Client(const Client&);
		Client&	operator=(const Client&);

		void	_process_method(Response* res, int socket, int& state);

		const Server*	_server;
		Request*	_request;
};

const inline Request*	Client::get_request() const
{ return _request; }

const inline Server*	Client::get_server() const
{ return _server; }

inline void	Client::set_server(const Server* server)
{ _server = server; }

#endif
