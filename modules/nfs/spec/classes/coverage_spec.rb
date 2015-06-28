#
# activate coverage calculation for the whole project
#
at_exit { RSpec::Puppet::Coverage.report! }
