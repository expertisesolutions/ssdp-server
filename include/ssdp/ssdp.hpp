#ifndef SSDP_SERVER_SSDP_SSDP_HPP
#define SSDP_SERVER_SSDP_SSDP_HPP

#include <boost/http/syntax/field_name.hpp>
#include <boost/http/syntax/field_value.hpp>
#include <boost/http/syntax/crlf.hpp>
#include <boost/asio.hpp>

#include <list>

#include <boost/spirit/home/x3.hpp>

namespace ssdp {

struct listener {

  listener(boost::asio::io_service& service, unsigned short port)
    : socket(service, boost::asio::ip::udp::v4()), port(port)
  {
    socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  }

  void bind()
  {
    socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
    asynchronous_wait();
  }

  void join_group(boost::asio::ip::address const& group)
  {
    socket.set_option(boost::asio::ip::multicast::join_group(group));
  }
private:
  struct task
  {
    boost::asio::ip::udp::endpoint remote_endpoint;
    std::array<char, 1500> static_buffer;
  } pending_task;
  
  void asynchronous_wait()
  {
    socket.async_receive_from
      (boost::asio::buffer(&pending_task.static_buffer[0], pending_task.static_buffer.size())
       , pending_task.remote_endpoint
       , [this] (auto const& ec, auto size)
       {
         if(!ec)
         {
           std::cout << "received " << std::endl;
           std::copy(pending_task.static_buffer.begin(), pending_task.static_buffer.begin() + size
                     , std::ostream_iterator<char>(std::cout));
           
           namespace x3 = boost::spirit::x3;
         
           auto iterator = pending_task.static_buffer.begin(),
             last = pending_task.static_buffer.begin() + size;
           if(x3::parse(iterator, last, "M-SEARCH * HTTP/1.1\r\n"))
           {
             std::cout << "is a M-SEARCH" << std::endl;

             std::vector<std::pair<std::string, std::string>> headers;
             boost::string_ref rest_message(&*iterator, size - (iterator - pending_task.static_buffer.begin()));

             boost::http::syntax::strict_crlf<char> crlf;
             size_t match_size = 0;

             while(!(match_size = crlf.match(rest_message)) && !rest_message.empty())
               {
                 rest_message.remove_prefix(match_size);
                 
                 boost::http::syntax::field_name<char> name;
                 boost::http::syntax::left_trimmed_field_value<char> value;

                 
                 if((match_size = name.match(rest_message)))
                 {
                   headers.push_back({{rest_message.begin(), rest_message.begin() + match_size}, {}});
                   std::cout << "name " << headers.back().first << std::endl;
                   
                   rest_message.remove_prefix(match_size+1);
                   if((match_size = value.match(rest_message)))
                   {
                     headers.back().second = {rest_message.begin(), rest_message.begin() + match_size};
                     rest_message.remove_prefix(match_size);

                     std::cout << "new header " << headers.back().first << ":" << headers.back().second << std::endl;
                     if(!(match_size = crlf.match(rest_message)))
                     {
                       std::cout << "failed reading header value" << std::endl;
                       break;
                     }
                     else
                     {
                       rest_message.remove_prefix(match_size);
                       // trigger call
                     }
                   }
                   else
                   {
                     std::cout << "failed reading field value" << std::endl;
                     break;
                   }
                 }
                 else
                 {
                   std::cout << "failed reading field name" << std::endl;
                   break;
                 }
               }

             std::cout << "all headers are read or error" << std::endl;
           }
           else
           {
             std::cout << "ignored message" << std::endl;
           }
         }
         else
           std::cout << "Error " << ec << std::endl;

         std::exit(-1);
       });
    

    // ssdp_socket->async_receive_from
    //   (boost::asio::mutable_buffers_1(&ssdp_task->buffer[0], ssdp_task->buffer.size())
    //    , ssdp_task->remote_endpoint
    //  // , [ssdp_task] (boost::system::error_code const& ec, std::size_t size)
    //  // {
    //  //   return x3::parse(ssdp_task->buffer.begin(), ssdp_task->buffer.begin() + size
    //  //                    , x3::lit("M-SEARCH * HTTP/1.1\r\n")
    //  //                    >> *http_parsers::http::header
    //  //                    >> x3::lit("\r\n")
    //  //                    );
    //  // }
    //  , [=] (boost::system::error_code const& ec, std::size_t size)
    //  {
    //    std::cout << "received ssdp packet" << std::endl;
    //    // std::copy(ssdp_task->buffer.begin(), ssdp_task->buffer.begin() + size, std::ostream_iterator<char>(std::cout));
    //    // std::endl(std::cout);

    //    if(x3::parse(ssdp_task->buffer.begin(), ssdp_task->buffer.begin() + size
    //                 , x3::lit("M-SEARCH * HTTP/1.1\r\n")))
    //    {
    //      std::cout << "sending response" << std::endl;
    //      for(auto& response : responses)
    //        ssdp_socket->send_to
    //          (boost::asio::const_buffers_1(&response[0], response.size())
    //           , ssdp_task->remote_endpoint);
    //    }
  }

  boost::asio::ip::udp::socket socket;
  unsigned short port;
  std::vector<std::string> responses;
};
  
}
  
#endif
