# This function generates a list of clients and options suitable for
# handing to the nfs::export type. It takes:
#  clients - An array of client hosts   ex: ['some_host.dom.com']
#  options - A hash of client -> client_options  
#           ex: {'some_host.dom.com' => 'ro'}
#  default - The default options string for when no explicit 
#            options are set for a given client.  ex: 'ro'
# It returns a space separated list of clients(options) for each client.
# 
# In puppet, it allows us to do this:
#
# $clients = {
#       'hosta.dom.com' => 'ro',
#       'hostb.dom.com' => 'rw,no_root_squash',
#       'hostc.dom.com' => 'ro'}
#
# nfs::server::export {'/data':
#   clients => mk_client_list(keys($clients), $clients, ''),
# etc...
#
# Or in this style:
#
# $clients = ['hosta.dom.com', 'hostb.dom.com', ...]
# nfs::server::export {'/data':
#   clients => mk_client_list($clients, {}, 'rw,no_root_squash')
#
# Or some combination of the two, like a client array with a separate hash
# for just those hosts with non-default options.
def mk_client_list(clients, options, default, err)

    # Make clients into an array if it isn't already.
    if not clients.kind_of?(Array) 
        clients = [clients]
    end

    client_str = []

    clients.each { |client|
        if options.is_a?(Hash) and options.has_key?(client)
            client_options = options[client]
        else
            client_options = default
        end
        client_str.push("%s(%s)" % [client, client_options])
    }

    return client_str.join(" ")
end


if __FILE__ == $0
    err_proc = Proc.new { |err_txt|
        puts err_txt
    }

    puts mk_client_list(['hosta', 'hostb'], {'hosta'=>'blah'}, 'ro', err_proc)

else
    module Puppet::Parser::Functions
        newfunction(:mk_client_list, :type=>:rvalue) do |args|
            Puppet::Parser::Functions.autoloader.loadall()

            clients = args[0]
            options = args[1]
            default_option = args[2]
            err_proc = Proc.new { |err_txt|
                function_err([err_txt])
            }

            return mk_client_list(clients, options, default_option, err_proc)
        end
    end
end
