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

package ceph

import (
	"log"
	"os"
	"strings"
	"testing"
)

var fixtures map[command][]byte

const cmdBroken command = "broken"

func TestMain(m *testing.M) {
	fixtures = make(map[command][]byte)

	for _, cmd := range []command{
		cmdDf, cmdPgDump, cmdOSDCrushRuleDump, cmdOSDCrushTree, cmdOSDDump, cmdHealth,
	} {
		var err error
		fixtures[cmd], err = os.ReadFile(
			"testdata/" + strings.ReplaceAll(string(cmd), " ", "_") + ".json",
		)
		if err != nil {
			log.Fatal(err)
		}
	}

	fixtures[cmdBroken] = []byte{1, 2, 3, 4, 5}

	os.Exit(m.Run())
}
