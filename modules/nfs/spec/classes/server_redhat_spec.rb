require 'spec_helper'
describe 'nfs::server::redhat' do
  context "operatingsystemrelease => 6.4" do
    let(:facts) { {:operatingsystemrelease => '6.4'} }
    it do
      should contain_class('nfs::client::redhat')
      should contain_service('nfs').with( 'ensure' => 'running'  )
    end
  end
  
  context "operatingsystemrelease => 7.1" do
    let(:facts) { {:operatingsystemrelease => '7.1'} }
    it do
      should contain_class('nfs::client::redhat')
      should contain_class('nfs::server::redhat::service')
      should contain_service('nfs-server').with( 'ensure' => 'running'  )
    end

    context ":nfs_v4 => true" do
      let(:params) {{ :nfs_v4 => true , :nfs_v4_idmap_domain => 'teststring' }}
      it do
        should contain_augeas('/etc/idmapd.conf').with_changes(/set Domain teststring/)
      end
    end

    context "setting mountd port" do
      let(:params) {{ :mountd_port => 4711 }}
      it do
        expect {
          should contain_class('nfs::server::redhat')
        }.to raise_error(Puppet::Error, /Setting mountd port currently not supported on RedHat/)
      end
    end
  end
end
