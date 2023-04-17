/*
** Zabbix
** Copyright (C) 2001-2023 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

package postgres

import (
	"context"
	"net/http"
	"net/url"
	"time"

	"zabbix.com/pkg/metric"
	"zabbix.com/pkg/tlsconfig"
	"zabbix.com/pkg/uri"
	"zabbix.com/pkg/zbxerr"

	"github.com/omeid/go-yarn"

	"zabbix.com/pkg/plugin"
)

const (
	pluginName = "Postgres"
	sqlExt     = ".sql"
	hkInterval = 10
)

// Plugin inherits plugin.Base and store plugin-specific data.
type Plugin struct {
	plugin.Base
	connMgr *ConnManager
	options PluginOptions
}

// impl is the pointer to the plugin implementation.
var impl Plugin

// Export implements the Exporter interface.
func (p *Plugin) Export(key string, rawParams []string, _ plugin.ContextProvider) (result interface{}, err error) {
	params, extraParams, err := metrics[key].EvalParams(rawParams, p.options.Sessions)
	if err != nil {
		return nil, err
	}

	err = metric.SetDefaults(params, p.options.Default)
	if err != nil {
		return nil, err
	}

	details, err := tlsconfig.CreateDetails(params["sessionName"], params["TLSConnect"],
		params["TLSCAFile"], params["TLSCertFile"], params["TLSKeyFile"], params["URI"])
	if err != nil {
		return nil, zbxerr.ErrorInvalidConfiguration.Wrap(err)
	}

	dbname := url.QueryEscape(params["Database"])

	uri, err := uri.NewWithCreds(params["URI"]+"?dbname="+dbname, params["User"], params["Password"], uriDefaults)
	if err != nil {
		return nil, err
	}

	handleMetric := getHandlerFunc(key)
	if handleMetric == nil {
		return nil, zbxerr.ErrorUnsupportedMetric
	}

	conn, err := p.connMgr.GetConnection(*uri, details)
	if err != nil {
		// Special logic of processing connection errors should be used if pgsql.ping is requested
		// because it must return pingFailed if any error occurred.
		if key == keyPing {
			return pingFailed, nil
		}

		p.Errf(err.Error())

		return nil, err
	}

	ctx, cancel := context.WithTimeout(conn.ctx, conn.callTimeout)
	defer cancel()

	result, err = handleMetric(ctx, conn, key, params, extraParams...)

	if err != nil {
		p.Errf(err.Error())
	}

	return result, err
}

// Start implements the Runner interface and performs initialization when plugin is activated.
func (p *Plugin) Start() {
	queryStorage, err := yarn.New(http.Dir(p.options.CustomQueriesPath), "*"+sqlExt)
	if err != nil {
		p.Errf(err.Error())
		// create empty storage if error occurred
		queryStorage = yarn.NewFromMap(map[string]string{})
	}

	p.connMgr = NewConnManager(
		time.Duration(p.options.KeepAlive)*time.Second,
		time.Duration(p.options.Timeout)*time.Second,
		time.Duration(p.options.CallTimeout)*time.Second,
		hkInterval*time.Second,
		queryStorage,
	)
}

// Stop implements the Runner interface and frees resources when plugin is deactivated.
func (p *Plugin) Stop() {
	p.connMgr.Destroy()
	p.connMgr = nil
}
