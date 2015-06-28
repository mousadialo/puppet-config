
require 'spec_helper'
describe 'nfs::server::debian', :type => :class do
  it do
    should contain_class('nfs::client::debian')
    should contain_class('nfs::server::debian::service')
    should contain_package('nfs-kernel-server')
    should contain_service('nfs-kernel-server').with( 'ensure' => 'running'  )
  end
  context ":nfs_v4 => true" do
    let(:params) {{ :nfs_v4 => true }}
    it do
      should contain_service('idmapd').with( 'ensure' => 'running'  )
    end

  end
  context "mountd params set" do
    let(:params) {{ :mountd_port => '4711' }}
    it do
      should contain_shellvar('rpc-mount-options') #.with( 'ensure' => 'present' )
    end

  end
end

