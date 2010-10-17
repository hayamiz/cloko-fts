# -*- coding: utf-8 -*-
# encoding: utf-8

Encoding.default_external = "UTF-8"

require 'optparse'
require 'socket'

class BasicSocket
  def send_all(buf, flags = 0)
    total = 0
    while buf && buf.size > 0
      sz = self.send(buf, flags)
      buf = buf.slice(sz..(buf.size))
      total += sz
    end

    total
  end

  def recv_all(len)
    ret = ""
    total = 0
    while total < len
      blk = self.read(len - total)
      if blk.nil? || blk.size == 0
        break
      end
      ret += blk
      total += blk.size
    end

    ret
  end
end

module FullTextSearch
  class Client
    def initialize(node, port = 30001)
      @node = node
      @port = port
      @sock = nil
    end

    def normalize_phrase(phrase)
      if phrase =~ /\n|\r/
        raise InvalidArgument.new("Query phrases must not contain new line chars.")
      end

      case phrase
      when /\A"(.+)"\Z/
        # replace '"' with multibyte char
        sprintf("\"%s\"", $~[1].gsub(/"/, "\342\200\235"))
      when /\A.+\Z/
        sprintf("\"%s\"", phrase.gsub(/"/, "\342\200\235"))
      else
        nil
      end
    end

    def request_queries(queries)
      unless @sock
        connect()
      end

      results = Array.new

      query_thread = Thread.new {
        queries.each do |query| # query is just an array of phrases
          cmd_query(query[:phrases])
        end
      }

      queries.each do |_|
        results.push(cmd_result())
      end

      query_thread.join
      disconnect()

      results
    end

    def connect
      return unless @sock.nil?
      @sock = TCPSocket.open(@node, @port)
    end

    def disconnect
      return if @sock.nil?
      cmd_bye()
      @sock.shutdown
      @sock.close
      @sock = nil
    end

    def cmd_query(query)
      query = query.map{|phrase| normalize_phrase(phrase)}.compact
      cmdstr = "QUERY " + query.join(" ") + "\n"

      @sock.send_all(cmdstr)
    end

    def cmd_result()
      unless result_head = @sock.gets
        raise RuntimeError.new("Socket error")
      end
      ret_doc_num = 0
      ret_size = 0
      case result_head
      when /\ARESULT (\d+) (\d+)\n/
        ret_doc_num = $~[1].to_i
        ret_size = $~[2].to_i
        result_head = $~[0]
      else
        raise RuntimeError.new("Invalid response for QUERY command: #{result_head.inspect}")
      end
      ret_body = @sock.recv_all(ret_size)

      {
        :num => ret_doc_num,
        :size => ret_size,
        :body => ret_body
      }
    end

    def cmd_bye
      return if @sock.nil?
      @sock.send_all("BYE\n")
    end

    def cmd_quit
      return if @sock.nil?
      @sock.send_all("QUIT\n")
    end
  end

  class Competition
    def initialize(node, port = 30001, output = $stdout)
      @node = node
      @port = port
      @output = output
      @client = Client.new(@node, @port)
    end

    def run(io_or_string, measure_time = false)
      t0 = Time.now
      queries = load_queries(io_or_string)
      @client.connect()
      rets = @client.request_queries(queries)
      rets.each_with_index do |ret, idx|
        @output.puts("## #{queries[idx][:id]} #{ret[:num]}")
        @output.puts(ret[:body])
      end
      t1 = Time.now

      if measure_time
        $stderr.printf("== %.3f [msec] ==\n", (t1 - t0) * 1000)
      end
    end

    def load_queries(io_or_string)
      if io_or_string.is_a? IO
        return load_queries(io_or_string.read)
      end
      str = io_or_string

      str.strip.split("\n").map{|line|
        if line =~ /\A(\d+) (.+)\Z/
          qid = $~[1].to_i
          phrases = []
          phrases_str = $~[2]
          phrases_str.gsub(/"[^"]+"/){|phrase|
            phrases << phrase
          }
          {:id => qid, :phrases => phrases}
        else
          nil
        end
      }.compact
    end

    def quit
      @client.connect()
      @client.cmd_quit()
    end
  end
end

if __FILE__ == $0

end
