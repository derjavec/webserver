#!/usr/bin/env ruby

puts "Content-Type: text/html\n\n"

method = ENV['REQUEST_METHOD'] || 'GET'

if method == 'POST'
  content_length = ENV['CONTENT_LENGTH'].to_i
  post_data = STDIN.read(content_length)

  name = 'Guest'
  if post_data.start_with?('name=')
    name = post_data.split('=').last
  end

  puts "<h1>Welcome, #{name}!</h1>"
else
  puts "<h1>Hello! This is a static message from RUBY.</h1>"
end
