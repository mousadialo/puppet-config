#!/usr/bin/env python

"""
Files related to encryption of passwords and keys.
"""

import sys
import os
import pwd
import lxml.etree
import struct
import base64
from Crypto.Cipher import AES
from PBKDF2 import PBKDF2

def FindFiles(root, pred):
  for item in [ os.path.join(root, x) for x in os.listdir(root) ]:
    if pred(item):
      yield item
    if os.path.isdir(item):
      for sub_item in FindFiles(item, pred):
        yield sub_item

def GetCryptoKey(salt, pw):
  """Use the PBKDF2 key strengthening function to turn the password
  itno an encryption key.  This is slow enough to compute that it
  provides some check against brute force search through the keyspace.
  """
  return PBKDF2(pw, salt).read(32)

def pad(x): 
  x += '\x00'
  k = (16 - (len(x) % 16)) % 16
  return x + (k * struct.pack('B', k))

def unpad(str):
  """Strings are padding with a null byte followed by n bytes of value
  n (binary).  This function removes the padding and the null byte"""
  padlen, = struct.unpack('B', str[-1])
  reallen = len(str) - padlen
  return str[0:reallen - 1]

CRYPT_ID = '\x43\xa4\x70'
def decrypt(key, pw):
  raw = base64.b64decode(pw)
  salt = raw[:8]
  rest = raw[8:]
  cryptor = AES.new(GetCryptoKey(salt, key), AES.MODE_CBC)
  pw = unpad(cryptor.decrypt(rest))
  if pw[:3] != CRYPT_ID:
    raise ValueError('Failed decryption identity check')
  return pw[3:]

def encrypt(key, pw):
  salt = os.urandom(8)
  pw = CRYPT_ID + pw
  cryptor = AES.new(GetCryptoKey(salt, key), AES.MODE_CBC)
  return base64.b64encode(salt + cryptor.encrypt(pad(pw)))

def touch(file, mode=0644):
  f = open(file, 'wb')
  f.close()
  os.chmod(file, mode)

def DecryptPropertiesXml(repo_root, decrypt_pw):
  public_file = os.path.join(repo_root, "etc", "properties.xml.safe")
  private_file = os.path.join(repo_root, "etc", "properties.xml")
  touch(private_file, mode=0600)
  properties = lxml.etree.parse(public_file)
  nodes = properties.getroot().xpath("/Properties/passwords/password")
  for node in nodes:
    key = node.attrib["name"]
    crypted_pw = node.attrib["password"]
    try:
      node.attrib["password"] = decrypt(decrypt_pw, crypted_pw)
    except Exception, e:
      sys.stderr.write("Error: %s\n" % str(e))
      node.attrib["pw"] = "ERR"

  private = open(private_file, 'w')
  private.write(lxml.etree.tostring(properties, pretty_print=True))
  private.close()

def DecryptSecureFiles(repo_root, decrypt_pw):
  for path in FindFiles(repo_root, lambda p: p.endswith(".secure")):
    DecryptSecureFile(path, decrypt_pw)

def DecryptSecureFile(path, decrypt_pw):
  file = open(path, 'r')
  private_path = path[:-7]
  touch(private_path, 0600)
  private_file = open(private_path, 'wb')
  private_file.write(decrypt(decrypt_pw, file.read()))
  file.close()
  private_file.close()

def EncryptSecureFile(crypto_pw, files):
  for file in files:
    tocrypt = open(file, 'rb').read()
    secure_file = open('%s.secure' % file, 'w')
    crypted = encrypt(crypto_pw, tocrypt)
    secure_file.write(crypted)
    secure_file.close()
