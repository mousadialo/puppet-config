# postfix configuration for HCS mail servers
class mail {
  include mail::postfix

  if machine_type == "mail" {
    include mail::dovecot
  }

}
