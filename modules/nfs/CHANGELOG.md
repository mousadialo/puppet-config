## 2015-06-05 - 1.5.0 (Feature/Bugfixe release)

#### Features:

- Issue #22 Make the name/ip of the server a config option in `nfs::server::export`
- Issue #24 More flexible nfsv4 export naming

#### Bugfixes:

- Fixed #20 fix client examples in README
- Fixed #21 Debian: fix service name fpr nfs v4
- Fixed #23 Default mount name in `client::mount` `should be undef, so it gets filled in the module
- Fixed #25 Fix tests for travis ci

## 2015-04-28 - 1.4.1 (Bugfixe release)

#### Bugfixes:

- Fixed #19 use of wrong variable for osfamily on servers

## 2015-04-22 - 1.4.0 (Feature/Bugfixe release)

#### Features:

- Issue #17 Add support for Amazon linux

#### Bugfixes:

- Fixed #16 revert default permissions from 0777 to 0755
- Fixed #18 problems with a host being client & server

## 2015-03-20 - 1.3.1 (Feature/Bugfixe release)

#### Features:

- Issue #11 Add function to easy handle large numer of clients (see README "A large number of clients")

#### Bugfixes:

- Fixed #13 RHEL 7: handle service enable correct
- Fixed #12 RHEL 7: ensure client mount works correct with undef mounts

## 2015-03-05 - 1.3.0 (Feature release)

#### Features:

- Allow mounting shares with the same name from different servers
- Add Scientific Linux CERN support

## 2015-02-16 - 1.2.1 (Bugfix release)

#### Bugfixes:

- ensure install dependencies for mount

## 2015-01-15 - 1.2.0 (Feature/Bugfix release)

#### Features:

- add support for gentoo
- add support for RHEL 7
- add support of puppet 3.x
- add possibility to set ownership of mountpoint
- lots of tests added

#### Behavior changes

- Parameter `tag` is now `nfstag`

## 2012-11.18 - 1.1.1

Original release of deprecated module haraldsk/nfs
