/*
** Copyright (C) 2001-2024 Zabbix SIA
**
** This program is free software: you can redistribute it and/or modify it under the terms of
** the GNU Affero General Public License as published by the Free Software Foundation, version 3.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
** without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU Affero General Public License for more details.
**
** You should have received a copy of the GNU Affero General Public License along with this program.
** If not, see <https://www.gnu.org/licenses/>.
**/

package agent

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"strings"
	"unicode"
	"unicode/utf8"

	"golang.zabbix.com/agent2/pkg/tls"
	"golang.zabbix.com/sdk/plugin"
)

var Options AgentOptions

const (
	HostNameLen      = 128
	hostNameListLen  = 2048
	HostMetadataLen  = 65535 // UTF-8 characters, not bytes
	HostInterfaceLen = 255   // UTF-8 characters, not bytes
	Variant          = 2
)

// CutAfterN returns the whole string s, if it is not longer then n runes (not bytes). Otherwise it returns the
// beginning of the string s, which is cut after the fist n runes.
func CutAfterN(s string, n int) string {
	var i int
	for pos := range s {
		if i >= n {
			return s[:pos]
		}
		i++
	}

	return s
}

func CheckHostnameParameter(s string) error {
	for i := 0; i < len(s); i++ {
		if s[i] == '.' || s[i] == ' ' || s[i] == '_' || s[i] == '-' || s[i] == ',' ||
			(s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= '0' && s[i] <= '9') {
			continue
		}

		if unicode.IsPrint(rune(s[i])) {
			return fmt.Errorf("character \"%c\" is not allowed in host name", s[i])
		} else {
			return fmt.Errorf("character 0x%02x is not allowed in host name", s[i])
		}
	}
	return nil
}

func ValidateHostnames(s string) ([]string, error) {
	hostnames := ExtractHostnames(s)
	keys := make(map[string]bool)
	huniq := []string{}
	for _, h := range hostnames {
		if h == "" {
			return nil, fmt.Errorf("host names cannot be empty")
		}
		if len(h) > HostNameLen {
			return nil, fmt.Errorf("host name in list is more than %d symbols", HostNameLen)
		}
		if _, value := keys[h]; !value {
			keys[h] = true
			huniq = append(huniq, h)
		}
	}
	if len(huniq) != len(hostnames) {
		return nil, fmt.Errorf("host names are not unique")
	}

	return hostnames, nil
}

func ExtractHostnames(s string) []string {
	hostnames := strings.Split(s, ",")

	for i := 0; i < len(hostnames); i++ {
		hostnames[i] = strings.Trim(hostnames[i], " \t")
	}

	return hostnames
}

func GetTLSConfig(options *AgentOptions) (cfg *tls.Config, err error) {
	if !tls.Supported() {
		if options.TLSAccept != "" ||
			options.TLSConnect != "" ||
			options.TLSPSKFile != "" ||
			options.TLSPSKIdentity != "" {
			return nil, errors.New(tls.SupportedErrMsg())
		}

		return
	}

	c := &tls.Config{}
	switch options.TLSConnect {
	case "", "unencrypted":
		c.Connect = tls.ConnUnencrypted
	case "psk":
		c.Connect = tls.ConnPSK
	case "cert":
		c.Connect = tls.ConnCert
	default:
		return nil, errors.New("invalid TLSConnect configuration parameter")
	}

	if options.TLSAccept != "" {
		opts := strings.Split(options.TLSAccept, ",")
		for _, o := range opts {
			switch strings.Trim(o, " \t") {
			case "unencrypted":
				c.Accept |= tls.ConnUnencrypted
			case "psk":
				c.Accept |= tls.ConnPSK
			case "cert":
				c.Accept |= tls.ConnCert
			default:
				return nil, errors.New("invalid TLSAccept configuration parameter")
			}
		}
	} else {
		c.Accept = tls.ConnUnencrypted
	}

	if c.Accept&(tls.ConnPSK|tls.ConnCert) == tls.ConnPSK|tls.ConnCert {
		if options.TLSCipherCert != "" {
			return nil, errors.New(`TLSCipherCert configuration parameter cannot be used when the combined list of` +
				` certificate and PSK ciphersuites are used. Use TLSCipherAll to configure certificate ciphers`)
		}
		if options.TLSCipherCert13 != "" {
			return nil, errors.New(`TLSCipherCert13 configuration parameter cannot be used when the combined list of` +
				` certificate and PSK ciphersuites are used. Use TLSCipherAll13 to configure certificate ciphers`)
		}
		c.CipherAll = options.TLSCipherAll
		c.CipherAll13 = options.TLSCipherAll13
	} else {
		if options.TLSCipherAll != "" {
			return nil, errors.New(`parameter "TLSCipherAll" cannot be applied: the combined list of certificate` +
				` and PSK ciphersuites is not used. Most likely parameters "TLSCipherCert" and/or "TLSCipherPSK"` +
				` are sufficient`)
		}

		if options.TLSCipherAll13 != "" {
			return nil, errors.New(`parameter "TLSCipherAll13" cannot be applied: the combined list of certificate` +
				` and PSK ciphersuites is not used. Most likely parameters "TLSCipherCert13" and/or "TLSCipherPSK13"` +
				` are sufficient`)
		}

		c.CipherAll = options.TLSCipherCert
		c.CipherAll13 = options.TLSCipherCert13
	}
	c.CipherPSK = options.TLSCipherPSK
	c.CipherPSK13 = options.TLSCipherPSK13

	if (c.Accept|c.Connect)&tls.ConnPSK != 0 {
		if options.TLSPSKIdentity != "" {
			c.PSKIdentity = options.TLSPSKIdentity
		} else {
			return nil, errors.New("missing TLSPSKIdentity configuration parameter")
		}
		if options.TLSPSKFile != "" {
			var file *os.File
			if file, err = os.Open(options.TLSPSKFile); err != nil {
				return nil, fmt.Errorf("invalid TLSPSKFile configuration parameter: %s", err)
			}
			defer file.Close()
			var b []byte
			if b, err = ioutil.ReadAll(file); err != nil {
				return nil, fmt.Errorf("invalid TLSPSKFile configuration parameter: %s", err)
			}
			c.PSKKey = string(bytes.TrimRight(b, "\r\n \t"))
		} else {
			return nil, errors.New("missing TLSPSKFile configuration parameter")
		}

		if options.TLSCipherPSK != "" {
			c.CipherPSK = options.TLSCipherPSK
		}
		if options.TLSCipherPSK13 != "" {
			c.CipherPSK13 = options.TLSCipherPSK13
		}

	} else {
		if options.TLSPSKIdentity != "" {
			return nil, errors.New("TLSPSKIdentity configuration parameter set without PSK being used")
		}
		if options.TLSPSKFile != "" {
			return nil, errors.New("TLSPSKFile configuration parameter set without PSK being used")
		}
		if options.TLSCipherPSK != "" {
			return nil, errors.New("TLSCipherPSK configuration parameter set without PSK being used")
		}
		if options.TLSCipherPSK13 != "" {
			return nil, errors.New("TLSCipherPSK13 configuration parameter set without PSK being used")
		}
	}

	if (c.Accept|c.Connect)&tls.ConnCert != 0 {
		if options.TLSCAFile != "" {
			c.CAFile = options.TLSCAFile
		} else {
			return nil, errors.New("missing TLSCAFile configuration parameter")
		}
		if options.TLSCertFile != "" {
			c.CertFile = options.TLSCertFile
		} else {
			return nil, errors.New("missing TLSCertFile configuration parameter")
		}
		if options.TLSKeyFile != "" {
			c.KeyFile = options.TLSKeyFile
		} else {
			return nil, errors.New("missing TLSKeyFile configuration parameter")
		}
		c.ServerCertIssuer = options.TLSServerCertIssuer
		c.ServerCertSubject = options.TLSServerCertSubject
		c.CRLFile = options.TLSCRLFile

		if options.TLSCipherCert != "" {
			c.CipherAll = options.TLSCipherCert
		}
		if options.TLSCipherCert13 != "" {
			c.CipherAll13 = options.TLSCipherCert13
		}

	} else {
		if options.TLSCAFile != "" {
			return nil, errors.New("TLSCAFile configuration parameter set without certificates being used")
		}
		if options.TLSCertFile != "" {
			return nil, errors.New("TLSCertFile configuration parameter set without certificates being used")
		}
		if options.TLSKeyFile != "" {
			return nil, errors.New("TLSKeyFile configuration parameter set without certificates being used")
		}
		if options.TLSServerCertIssuer != "" {
			return nil, errors.New("TLSServerCertIssuer configuration parameter set without certificates being used")
		}
		if options.TLSServerCertSubject != "" {
			return nil, errors.New("TLSServerCertSubject configuration parameter set without certificates being used")
		}
		if options.TLSCRLFile != "" {
			return nil, errors.New("TLSCRLFile configuration parameter set without certificates being used")
		}
		if options.TLSCipherCert != "" {
			return nil, errors.New("TLSCipherCert configuration parameter set without certificates being used")
		}
		if options.TLSCipherCert13 != "" {
			return nil, errors.New("TLSCipherCert13 configuration parameter set without certificates being used")
		}
	}
	return c, nil
}

func GlobalOptions(all *AgentOptions) (options *plugin.GlobalOptions) {
	options = &plugin.GlobalOptions{
		Timeout:  Options.Timeout,
		SourceIP: Options.SourceIP,
	}
	return
}

func ValidateOptions(options *AgentOptions) error {
	var err error
	var maxLen int

	hosts := ExtractHostnames(options.Hostname)
	options.Hostname = strings.Join(hosts, ",")

	if len(hosts) > 1 {
		maxLen = hostNameListLen
	} else {
		maxLen = HostNameLen
	}

	if len(options.Hostname) > maxLen {
		return fmt.Errorf("the value of \"Hostname\" configuration parameter cannot be longer than %d"+
			" characters", maxLen)
	}
	if err = CheckHostnameParameter(options.Hostname); err != nil {
		return fmt.Errorf("invalid \"Hostname\" configuration parameter: %s", err.Error())
	}
	if utf8.RuneCountInString(options.HostInterface) > HostInterfaceLen {
		return fmt.Errorf("the value of \"HostInterface\" configuration parameter cannot be longer than %d"+
			" characters", HostInterfaceLen)
	}

	return nil
}
