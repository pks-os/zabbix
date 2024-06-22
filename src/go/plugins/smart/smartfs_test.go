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

package smart

import (
	"encoding/json"
	"errors"
	stdlog "log"
	"os"
	"testing"
	"time"

	"github.com/google/go-cmp/cmp"
	"golang.zabbix.com/agent2/plugins/smart/mock"
	"golang.zabbix.com/sdk/errs"
	"golang.zabbix.com/sdk/log"
	"golang.zabbix.com/sdk/plugin"
)

//nolint:paralleltest,tparallel
func Test_runner_executeBase(t *testing.T) {
	log.DefaultLogger = stdlog.New(os.Stdout, "", stdlog.LstdFlags)

	type expectation struct {
		args []string
		err  error
		out  []byte
	}

	type fields struct {
		devices     map[string]deviceParser
		jsonDevices map[string]jsonDevice
	}

	type args struct {
		basicDev   []deviceInfo
		jsonRunner bool
	}

	tests := []struct {
		name            string
		expectations    []expectation
		fields          fields
		args            args
		wantJsonDevices map[string]jsonDevice
		wantDevices     map[string]deviceParser
		wantErr         bool
	}{
		{
			"+valid",
			[]expectation{
				{
					args: []string{"-a", "/dev/sda", "-j"},
					err:  nil,
					out:  mock.OutputAllDiscInfoSDA,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "/dev/sda",
						InfoName: "/dev/sda",
						DevType:  "nvme",
					},
				},
				false,
			},
			map[string]jsonDevice{},
			map[string]deviceParser{
				"/dev/sda": {
					ModelName:    "SAMSUNG MZVL21T0HCLR-00BH1",
					SerialNumber: "S641NX0T509005",
					Info: deviceInfo{
						Name:     "/dev/sda",
						InfoName: "/dev/sda",
						DevType:  "nvme",
						name:     "/dev/sda",
					},
					Smartctl: smartctlField{
						Version: []int{7, 1},
					},
					SmartStatus:     &smartStatus{SerialNumber: true},
					SmartAttributes: smartAttributes{},
				},
			},
			false,
		},
		{
			"+multiple",
			[]expectation{
				{
					args: []string{"-a", "/dev/sda", "-j"},
					err:  nil,
					out:  mock.OutputAllDiscInfoSDA,
				},
				{
					args: []string{
						"-a",
						"IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
						"-j",
					},
					err: nil,
					out: mock.OutputAllDiscInfoMac,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "/dev/sda",
						InfoName: "/dev/sda",
						DevType:  "nvme",
					},
					{
						Name:     "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
						InfoName: "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
						DevType:  "nvme",
					},
				},
				false,
			},
			map[string]jsonDevice{},
			map[string]deviceParser{
				"/dev/sda": {
					ModelName:    "SAMSUNG MZVL21T0HCLR-00BH1",
					SerialNumber: "S641NX0T509005",
					Info: deviceInfo{
						Name:     "/dev/sda",
						InfoName: "/dev/sda",
						DevType:  "nvme",
						name:     "/dev/sda",
					},
					Smartctl: smartctlField{
						Version: []int{7, 1},
					},
					SmartStatus:     &smartStatus{SerialNumber: true},
					SmartAttributes: smartAttributes{},
				},
				"IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1": { //nolint:lll
					ModelName:    "APPLE SSD AP0512Z",
					SerialNumber: "0ba02202c4bc1a1e",
					Info: deviceInfo{
						Name:     "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
						InfoName: "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
						DevType:  "nvme",
						name:     "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
					},
					Smartctl: smartctlField{
						Version:    []int{7, 4},
						ExitStatus: 4,
						Messages: []message{
							{
								"Read 1 entries from Error Information Log failed: GetLogPage failed: system=0x38, sub=0x0, code=745", //nolint:lll
							},
						},
					},
					SmartStatus:     &smartStatus{SerialNumber: true},
					SmartAttributes: smartAttributes{},
				},
			},
			false,
		},
		{
			"+jsonRunner",
			[]expectation{
				{
					args: []string{"-a", "/dev/sda", "-j"},
					err:  nil,
					out:  mock.OutputAllDiscInfoSDA,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "/dev/sda",
						InfoName: "/dev/sda",
						DevType:  "nvme",
					},
				},
				true,
			},
			map[string]jsonDevice{
				"/dev/sda": {
					serialNumber: "S641NX0T509005",
					jsonData:     string(mock.OutputAllDiscInfoSDA),
				},
			},
			map[string]deviceParser{},
			false,
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			cpuCount = 1

			m := &mock.MockController{}

			for _, e := range tt.expectations {
				m.ExpectExecute().
					WithArgs(e.args...).
					WillReturnOutput(e.out).
					WillReturnError(e.err)
			}

			log.New("test")

			r := &runner{
				plugin: &Plugin{
					ctl:  m,
					Base: plugin.Base{Logger: log.New("test")},
				},
				names:       make(chan string, 10),
				err:         make(chan error, 10),
				done:        make(chan struct{}),
				devices:     tt.fields.devices,
				jsonDevices: tt.fields.jsonDevices,
			}

			defer func() {
				close(r.err)
			}()

			err := r.executeBase(tt.args.basicDev, tt.args.jsonRunner)
			if (err != nil) != tt.wantErr {
				t.Fatalf(
					"runner.executeBase() error = %v, wantErr %v",
					err,
					tt.wantErr,
				)
			}

			if diff := cmp.Diff(
				tt.wantJsonDevices, r.jsonDevices,
				cmp.AllowUnexported(jsonDevice{}),
			); diff != "" {
				t.Fatalf(
					"runner.executeBase() jsonDevices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if diff := cmp.Diff(
				tt.wantDevices, r.devices,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"runner.executeBase() devices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if err := m.ExpectationsWhereMet(); err != nil {
				t.Fatalf(
					"runner.executeBase() expectations where not met, error = %v",
					err,
				)
			}
		})
	}
}

//nolint:paralleltest,tparallel
func Test_runner_executeRaids(t *testing.T) {
	log.DefaultLogger = stdlog.New(os.Stdout, "", stdlog.LstdFlags)
	cpuCount = 1

	//nolint:lll
	sampleFailedAllSmartInfoScan := []byte(
		`{
        "json_format_version": [1, 0],
        "smartctl": {
          "version": [7, 3],
          "svn_revision": "5338",
          "platform_info": "x86_64-w64-mingw32-2016-1607",
          "build_info": "(sf-7.3-1)",
          "argv": ["smartctl", "-a", "/dev/sda", "-d", "3ware,0", "-j"],
          "messages": [
            {
              "string": "/dev/sda: Unknown device type '3ware,0'",
              "severity": "error"
            },
            {
              "string": "=======> VALID ARGUMENTS ARE: ata, scsi[+TYPE], nvme[,NSID], sat[,auto][,N][+TYPE], usbcypress[,X], usbjmicron[,p][,x][,N], usbprolific, usbsunplus, sntasmedia, sntjmicron[,NSID], sntrealtek, intelliprop,N[+TYPE], jmb39x[-q],N[,sLBA][,force][+TYPE], jms56x,N[,sLBA][,force][+TYPE], aacraid,H,L,ID, areca,N[/E], auto, test <=======",
              "severity": "error"
            }
          ],
          "exit_status": 1
        },
        "local_time": {
          "time_t": 1663357978,
          "asctime": "Fri Sep 16 22:52:58 2022 BST"
        }
      }`,
	)

	type expectation struct {
		args []string
		err  error
		out  []byte
	}

	type fields struct {
		devices     map[string]deviceParser
		jsonDevices map[string]jsonDevice
	}

	type args struct {
		raids      []deviceInfo
		jsonRunner bool
	}

	tests := []struct {
		name            string
		expectations    []expectation
		fields          fields
		args            args
		wantJsonDevices map[string]jsonDevice
		wantDevices     map[string]deviceParser
	}{
		{
			"+valid",
			[]expectation{
				{
					args: []string{
						"-a", "/dev/sda", "-d", "3ware,0", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d 3ware,0 -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "areca,1", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d areca,1 -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "cciss,0", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d cciss,0 -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "sat", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d sat -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "scsi", "-j",
					},
					out: sampleFailedAllSmartInfoScan,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "/dev/sda",
						InfoName: "/dev/sda [SAT]",
						DevType:  "sat",
					},
				},
				false,
			},
			map[string]jsonDevice{},
			map[string]deviceParser{
				"/dev/sda sat": {
					ModelName:    "INTEL SSDSC2BB120G6",
					SerialNumber: "PHWA619301M9120CGN",
					Info: deviceInfo{
						Name:     "/dev/sda sat",
						InfoName: "/dev/sda [SAT]",
						DevType:  "sat",
						name:     "/dev/sda",
						raidType: "sat",
					},
					Smartctl:    smartctlField{Version: []int{7, 3}},
					SmartStatus: &smartStatus{SerialNumber: true},
					SmartAttributes: smartAttributes{ //nolint:dupl
						Table: []table{
							{Attrname: "Reallocated_Sector_Ct", ID: 5},
							{Attrname: "Power_On_Hours", ID: 9},
							{Attrname: "Power_Cycle_Count", ID: 12},
							{
								Attrname: "Available_Reservd_Space",
								ID:       170,
								Thresh:   10,
							},
							{Attrname: "Program_Fail_Count", ID: 171},
							{Attrname: "Erase_Fail_Count", ID: 172},
							{Attrname: "Unsafe_Shutdown_Count", ID: 174},
							{
								Attrname: "Power_Loss_Cap_Test",
								ID:       175,
								Thresh:   10,
							},
							{Attrname: "SATA_Downshift_Count", ID: 183},
							{Attrname: "End-to-End_Error", ID: 184, Thresh: 90},
							{Attrname: "Reported_Uncorrect", ID: 187},
							{Attrname: "Temperature_Case", ID: 190},
							{Attrname: "Unsafe_Shutdown_Count", ID: 192},
							{Attrname: "Temperature_Internal", ID: 194},
							{Attrname: "Current_Pending_Sector", ID: 197},
							{Attrname: "CRC_Error_Count", ID: 199},
							{Attrname: "Host_Writes_32MiB", ID: 225},
							{Attrname: "Workld_Media_Wear_Indic", ID: 226},
							{Attrname: "Workld_Host_Reads_Perc", ID: 227},
							{Attrname: "Workload_Minutes", ID: 228},
							{
								Attrname: "Available_Reservd_Space",
								ID:       232,
								Thresh:   10,
							},
							{Attrname: "Media_Wearout_Indicator", ID: 233},
							{Attrname: "Thermal_Throttle", ID: 234},
							{Attrname: "Host_Writes_32MiB", ID: 241},
							{Attrname: "Host_Reads_32MiB", ID: 242},
							{Attrname: "NAND_Writes_32MiB", ID: 243},
						},
					},
				},
			},
		},
		{
			"+env1JSON",
			[]expectation{
				{
					args: []string{
						"-a", "/dev/sda", "-d", "3ware,0", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d 3ware,0 -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "areca,1", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d areca,1 -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "cciss,0", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d cciss,0 -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "sat", "-j",
					},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d sat -j"),
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "scsi", "-j",
					},
					out: sampleFailedAllSmartInfoScan,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "/dev/sda",
						InfoName: "/dev/sda [SAT]",
						DevType:  "sat",
					},
				},
				true,
			},
			map[string]jsonDevice{
				"/dev/sda sat": {
					serialNumber: "PHWA619301M9120CGN",
					jsonData: string(
						mock.Outputs.Get("env_1").
							AllSmartInfoScans.Get("-a /dev/sda -d sat -j"),
					),
				},
			},
			map[string]deviceParser{},
		},
		{
			"+HBA_with_SAS_1",
			[]expectation{
				{
					args: []string{
						"-a", "/dev/sda", "-d", "3ware,0", "-j",
					},
					out: sampleFailedAllSmartInfoScan,
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "areca,1", "-j",
					},
					out: sampleFailedAllSmartInfoScan,
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "cciss,0", "-j",
					},
					out: sampleFailedAllSmartInfoScan,
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "sat", "-j",
					},
					out: sampleFailedAllSmartInfoScan,
				},
				{
					args: []string{
						"-a", "/dev/sda", "-d", "scsi", "-j",
					},
					out: mock.Outputs.Get("HBA_with_SAS_1").
						AllSmartInfoScans.Get("-a /dev/sda -d scsi -j"),
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{Name: "/dev/sda", InfoName: "/dev/sda", DevType: "scsi"},
				},
				false,
			},
			map[string]jsonDevice{},
			map[string]deviceParser{
				"/dev/sda scsi": {
					SerialNumber: "S5G1NC0W102239",
					Info: deviceInfo{
						Name:     "/dev/sda scsi",
						InfoName: "/dev/sda",
						DevType:  "scsi",
						name:     "/dev/sda",
						raidType: "scsi",
					},
					Smartctl:    smartctlField{Version: []int{7, 3}},
					SmartStatus: &smartStatus{SerialNumber: true},
				},
			},
		},
		{
			"-noDevices",
			[]expectation{},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{[]deviceInfo{}, false},
			map[string]jsonDevice{},
			map[string]deviceParser{},
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			m := mock.NewMockController(t)

			for _, e := range tt.expectations {
				m.ExpectExecute().
					WithArgs(e.args...).
					WillReturnOutput(e.out).
					WillReturnError(e.err)
			}

			r := &runner{
				plugin: &Plugin{
					ctl:  m,
					Base: plugin.Base{Logger: log.New("test")},
				},
				raidDone:    make(chan struct{}),
				devices:     tt.fields.devices,
				jsonDevices: tt.fields.jsonDevices,
			}

			r.executeRaids(tt.args.raids, tt.args.jsonRunner)

			if diff := cmp.Diff(
				tt.wantJsonDevices, r.jsonDevices,
				cmp.AllowUnexported(jsonDevice{}),
			); diff != "" {
				t.Fatalf(
					"runner.executeRaids() jsonDevices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if diff := cmp.Diff(
				tt.wantDevices, r.devices,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"runner.executeRaids() devices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if err := m.ExpectationsWhereMet(); err != nil {
				t.Fatalf(
					"runner.executeRaids() expectations where not met, error = %v",
					err,
				)
			}
		})
	}
}

//nolint:paralleltest,tparallel
func Test_runner_executeMegaRaids(t *testing.T) {
	log.DefaultLogger = stdlog.New(os.Stdout, "", stdlog.LstdFlags)

	type expectation struct {
		args []string
		err  error
		out  []byte
	}

	type fields struct {
		devices     map[string]deviceParser
		jsonDevices map[string]jsonDevice
	}

	type args struct {
		megaraids  []deviceInfo
		jsonRunner bool
	}

	tests := []struct {
		name            string
		expectations    []expectation
		fields          fields
		args            args
		wantJsonDevices map[string]jsonDevice
		wantDevices     map[string]deviceParser
	}{
		{
			"+valid",
			[]expectation{
				{
					args: []string{
						"-a", "optimus_prime", "-d", "transformer", "-j",
					},
					err: nil,
					out: mock.OutputAllDiscInfoSDA,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "optimus_prime",
						InfoName: "promtimus_ome",
						DevType:  "transformer",
						name:     "optimus_prime_name",
						raidType: "",
					},
				},
				false,
			},
			map[string]jsonDevice{},
			map[string]deviceParser{
				"optimus_prime transformer": {
					ModelName:    "SAMSUNG MZVL21T0HCLR-00BH1",
					SerialNumber: "S641NX0T509005",
					Info: deviceInfo{
						Name:     "optimus_prime transformer",
						InfoName: "/dev/sda",
						DevType:  "nvme",
						name:     "optimus_prime",
						raidType: "transformer",
					},
					Smartctl: smartctlField{
						Version: []int{7, 1},
					},
					SmartStatus:     &smartStatus{SerialNumber: true},
					SmartAttributes: smartAttributes{},
				},
			},
		},
		{
			"+JSONRunner",
			[]expectation{
				{
					args: []string{
						"-a", "optimus_prime", "-d", "transformer", "-j",
					},
					err: nil,
					out: mock.OutputAllDiscInfoSDA,
				},
			},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{
				[]deviceInfo{
					{
						Name:     "optimus_prime",
						InfoName: "promtimus_ome",
						DevType:  "transformer",
						name:     "optimus_prime_name",
						raidType: "",
					},
				},
				true,
			},
			map[string]jsonDevice{
				"optimus_prime transformer": {
					serialNumber: "S641NX0T509005",
					jsonData:     string(mock.OutputAllDiscInfoSDA),
				},
			},
			map[string]deviceParser{},
		},
		{
			"-noDevices",
			[]expectation{},
			fields{
				jsonDevices: map[string]jsonDevice{},
				devices:     map[string]deviceParser{},
			},
			args{},
			map[string]jsonDevice{},
			map[string]deviceParser{},
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			cpuCount = 1

			m := &mock.MockController{}

			for _, e := range tt.expectations {
				m.ExpectExecute().
					WithArgs(e.args...).
					WillReturnOutput(e.out).
					WillReturnError(e.err)
			}

			r := &runner{
				plugin: &Plugin{
					ctl:  m,
					Base: plugin.Base{Logger: log.New("test")},
				},
				megaRaidDone: make(chan struct{}),
				megaraids:    make(chan raidParameters, 10),
				devices:      tt.fields.devices,
				jsonDevices:  tt.fields.jsonDevices,
			}

			r.executeMegaRaids(tt.args.megaraids, tt.args.jsonRunner)

			if diff := cmp.Diff(
				tt.wantJsonDevices, r.jsonDevices,
				cmp.AllowUnexported(jsonDevice{}),
			); diff != "" {
				t.Fatalf(
					"runner.executeMegaRaids() jsonDevices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if diff := cmp.Diff(
				tt.wantDevices, r.devices,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"runner.executeMegaRaids() devices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if err := m.ExpectationsWhereMet(); err != nil {
				t.Fatalf(
					"runner.executeMegaRaids() expectations where not met, error = %v",
					err,
				)
			}
		})
	}
}

func Test_evaluateVersion(t *testing.T) {
	type args struct {
		versionDigits []int
	}
	tests := []struct {
		name    string
		args    args
		wantErr bool
	}{
		{"+correct_version", args{[]int{7, 1}}, false},
		{"+correct_version_one_digit", args{[]int{8}}, false},
		{"+correct_version_multiple_digits", args{[]int{7, 1, 2}}, false},
		{"-incorrect_version", args{[]int{7, 0}}, true},
		{"-malformed_version", args{[]int{-7, 0}}, true},
		{"-empty", args{}, true},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if err := evaluateVersion(tt.args.versionDigits); (err != nil) != tt.wantErr {
				t.Errorf(
					"evaluateVersion() error = %v, wantErr %v",
					err,
					tt.wantErr,
				)
			}
		})
	}
}

func Test_cutPrefix(t *testing.T) {
	type args struct {
		in string
	}
	tests := []struct {
		name string
		args args
		want string
	}{
		{"+has_prefix", args{"/dev/sda"}, "sda"},
		{"-no_prefix", args{"sda"}, "sda"},
		{"-empty", args{""}, ""},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := cutPrefix(tt.args.in); got != tt.want {
				t.Errorf("cutPrefix() = %v, want %v", got, tt.want)
			}
		})
	}
}

func Test_deviceParser_checkErr(t *testing.T) {
	type fields struct {
		Smartctl smartctlField
		rawResp  []byte
	}

	tests := []struct {
		name    string
		fields  fields
		wantErr bool
		wantMsg string
	}{
		{
			"+no_err",
			fields{Smartctl: smartctlField{Messages: nil, ExitStatus: 0}},
			false,
			"",
		},
		{
			"+rawRespErr",
			fields{
				rawResp: mock.Outputs.Get("env_1").
					AllSmartInfoScans.Get("-a /dev/sda -d 3ware,0 -j"),
			},
			true,
			"/dev/sda: Unknown device type '3ware,0', =======> VALID " +
				"ARGUMENTS ARE: ata, scsi[+TYPE], nvme[,NSID], " +
				"sat[,auto][,N][+TYPE], usbcypress[,X], " +
				"usbjmicron[,p][,x][,N], usbprolific, usbsunplus, " +
				"sntasmedia, sntjmicron[,NSID], sntrealtek, " +
				"intelliprop,N[+TYPE], jmb39x[-q],N[,sLBA][,force][+TYPE], " +
				"jms56x,N[,sLBA][,force][+TYPE], aacraid,H,L,ID, areca,N[/E], " +
				"auto, test <=======.",
		},
		{
			"+rawRespNoErr",
			fields{
				rawResp: mock.Outputs.Get("env_1").
					AllSmartInfoScans.Get("-a /dev/sda -d sat -j"),
			},
			false,
			"",
		},
		{
			"+no_err",
			fields{Smartctl: smartctlField{Messages: nil, ExitStatus: 4}},
			false,
			"",
		},
		{
			"+warning",
			fields{
				Smartctl: smartctlField{
					Messages: []message{{"barfoo"}}, ExitStatus: 3,
				},
			},
			true,
			"Barfoo.",
		},
		{
			"-error_status_one",
			fields{
				Smartctl: smartctlField{
					Messages: []message{{"barfoo"}}, ExitStatus: 1,
				},
			},
			true,
			"Barfoo.",
		},
		{
			"-error_status_two",
			fields{
				Smartctl: smartctlField{
					Messages: []message{{"foobar"}}, ExitStatus: 2,
				},
			},
			true,
			"Foobar.",
		},
		{
			"-two_err",
			fields{
				Smartctl: smartctlField{
					Messages: []message{{"foobar"}, {"barfoo"}}, ExitStatus: 2,
				},
			},
			true,
			"Foobar, barfoo.",
		},
		{
			"-unknown_err/no message",
			fields{
				Smartctl: smartctlField{
					Messages: []message{}, ExitStatus: 2,
				},
			},
			true,
			"Unknown error from smartctl.",
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			dp := deviceParser{Smartctl: tt.fields.Smartctl}

			if tt.fields.rawResp != nil {
				dpFromRaw := deviceParser{}

				err := json.Unmarshal(tt.fields.rawResp, &dpFromRaw)
				if err != nil {
					t.Fatalf("deviceParser.checkErr() error = %v", err)
				}

				dp = dpFromRaw
			}

			err := dp.checkErr()
			if (err != nil) != tt.wantErr {
				t.Fatalf(
					"deviceParser.checkErr() error = %v, wantErr %v",
					err,
					tt.wantErr,
				)
			}

			if err != nil {
				if diff := cmp.Diff(tt.wantMsg, err.Error()); diff != "" {
					t.Fatalf(
						"deviceParser.checkErr() error message mismatch (-want +got):\n%s",
						diff,
					)
				}
			}
		})
	}
}

//nolint:paralleltest
func TestPlugin_checkVersion(t *testing.T) {
	type expect struct {
		exec bool
	}

	type fields struct {
		execErr      error
		execOut      []byte
		lastVerCheck time.Time
	}

	tests := []struct {
		name    string
		expect  expect
		fields  fields
		wantErr bool
	}{
		{
			"+valid",
			expect{true},
			fields{execOut: mock.OutputVersionValid},
			false,
		},
		{
			"-noCheck",
			expect{false},
			fields{
				lastVerCheck: time.Now(),
			},
			false,
		},
		{
			"-executeErr",
			expect{true},
			fields{
				execOut: mock.OutputVersionValid,
				execErr: errors.New("fail"),
			},
			true,
		},
		{
			"-unmarshalErr",
			expect{true},
			fields{execOut: []byte("{")},
			true,
		},
		{
			"-evaluateVersionErr",
			expect{true},
			fields{execOut: mock.OutputVersionInvalid},
			true,
		},
	}
	//nolint:paralleltest
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			lastVerCheck = tt.fields.lastVerCheck

			m := mock.NewMockController(t)

			if tt.expect.exec {
				m.ExpectExecute().
					WithArgs("-j", "-V").
					WillReturnOutput(tt.fields.execOut).
					WillReturnError(tt.fields.execErr)
			}

			p := &Plugin{ctl: m}

			if err := p.checkVersion(); (err != nil) != tt.wantErr {
				t.Fatalf(
					"Plugin.checkVersion() error = %v, wantErr %v",
					err, tt.wantErr,
				)
			}

			if err := m.ExpectationsWhereMet(); err != nil {
				t.Fatalf(
					"Plugin.checkVersion() expectations where not met, error = %v",
					err,
				)
			}
		})
	}
}

func Test_getAllDeviceInfoByType(t *testing.T) {
	t.Parallel()

	sampleValidSmartInfoScan := []byte(`{
        "json_format_version": [1, 0],
        "smartctl": {
          "version": [7, 3],
          "svn_revision": "5338",
          "platform_info": "x86_64-linux-6.1.0-13-amd64",
          "build_info": "(local build)",
          "argv": ["smartctl", "-a", "-j", "/dev/sda", "-d", "scsi"],
          "exit_status": 0
        },
        "local_time": {
          "time_t": 1705569652,
          "asctime": "Thu Jan 18 10:20:52 2024 CET"
        },
        "device": {
          "name": "/dev/sda",
          "info_name": "/dev/sda",
          "type": "scsi",
          "protocol": "SCSI"
        },
        "scsi_vendor": "SAMSUNG",
        "scsi_product": "MZILT960HBHQ/007",
        "scsi_model_name": "SAMSUNG MZILT960HBHQ/007",
        "scsi_revision": "GXA0",
        "scsi_version": "SPC-5",
        "user_capacity": {
          "blocks": 1875385008,
          "bytes": 960197124096
        },
        "logical_block_size": 512,
        "physical_block_size": 4096,
        "scsi_lb_provisioning": {
          "name": "resource provisioned",
          "value": 1,
          "management_enabled": {
            "name": "LBPME",
            "value": 1
          },
          "read_zeros": {
            "name": "LBPRZ",
            "value": 1
          }
        },
        "rotation_rate": 0,
        "form_factor": {
          "scsi_value": 3,
          "name": "2.5 inches"
        },
        "logical_unit_id": "0x5002538b731302a0",
        "serial_number": "S5G1NC0W102239",
        "device_type": {
          "scsi_terminology": "Peripheral Device Type [PDT]",
          "scsi_value": 0,
          "name": "disk"
        },
        "scsi_transport_protocol": {
          "name": "SAS (SPL-4)",
          "value": 6
        },
        "smart_support": {
          "available": true,
          "enabled": true
        },
        "temperature_warning": {
          "enabled": true
        },
        "smart_status": {
          "passed": true
        },
        "scsi_percentage_used_endurance_indicator": 0,
        "temperature": {
          "current": 35,
          "drive_trip": 74
        },
        "power_on_time": {
          "hours": 3862,
          "minutes": 30
        },
        "scsi_start_stop_cycle_counter": {
          "year_of_manufacture": "2023",
          "week_of_manufacture": "01",
          "accumulated_start_stop_cycles": 5,
          "specified_load_unload_count_over_device_lifetime": 0,
          "accumulated_load_unload_cycles": 0
        },
        "scsi_grown_defect_list": 0,
        "scsi_error_counter_log": {
          "read": {
            "errors_corrected_by_eccfast": 0,
            "errors_corrected_by_eccdelayed": 0,
            "errors_corrected_by_rereads_rewrites": 0,
            "total_errors_corrected": 0,
            "correction_algorithm_invocations": 0,
            "gigabytes_processed": "12.699",
            "total_uncorrected_errors": 0
          },
          "write": {
            "errors_corrected_by_eccfast": 0,
            "errors_corrected_by_eccdelayed": 0,
            "errors_corrected_by_rereads_rewrites": 0,
            "total_errors_corrected": 0,
            "correction_algorithm_invocations": 0,
            "gigabytes_processed": "6082.745",
            "total_uncorrected_errors": 0
          },
          "verify": {
            "errors_corrected_by_eccfast": 0,
            "errors_corrected_by_eccdelayed": 0,
            "errors_corrected_by_rereads_rewrites": 0,
            "total_errors_corrected": 0,
            "correction_algorithm_invocations": 0,
            "gigabytes_processed": "0.001",
            "total_uncorrected_errors": 0
          }
        },
        "scsi_pending_defects": {
          "count": 0
        },
        "scsi_self_test_0": {
          "code": {
            "value": 1,
            "string": "Background short"
          },
          "result": {
            "value": 0,
            "string": "Completed"
          },
          "power_on_time": {
            "hours": 5,
            "aka": "accumulated_power_on_hours"
          }
        },
        "scsi_extended_self_test_seconds": 3600
      }`)
	//nolint:lll
	sampleFailedSmartInfoScan := []byte(`{
        "json_format_version": [1, 0],
        "smartctl": {
          "version": [7, 3],
          "svn_revision": "5338",
          "platform_info": "x86_64-w64-mingw32-2016-1607",
          "build_info": "(sf-7.3-1)",
          "argv": ["smartctl", "-a", "/dev/sda", "-d", "3ware,0", "-j"],
          "messages": [
            {
              "string": "/dev/sda: Unknown device type '3ware,0'",
              "severity": "error"
            },
            {
              "string": "=======> VALID ARGUMENTS ARE: ata, scsi[+TYPE], nvme[,NSID], sat[,auto][,N][+TYPE], usbcypress[,X], usbjmicron[,p][,x][,N], usbprolific, usbsunplus, sntasmedia, sntjmicron[,NSID], sntrealtek, intelliprop,N[+TYPE], jmb39x[-q],N[,sLBA][,force][+TYPE], jms56x,N[,sLBA][,force][+TYPE], aacraid,H,L,ID, areca,N[/E], auto, test <=======",
              "severity": "error"
            }
          ],
          "exit_status": 1
        },
        "local_time": {
          "time_t": 1663357978,
          "asctime": "Fri Sep 16 22:52:58 2022 BST"
        }
      }`)
	sampleMissingSmartStatusSmartInfoScan := []byte(`{
        "json_format_version": [1, 0],
        "smartctl": {
          "version": [7, 3],
          "svn_revision": "5338",
          "platform_info": "x86_64-linux-6.1.0-13-amd64",
          "build_info": "(local build)",
          "argv": ["smartctl", "-a", "-j", "/dev/sda", "-d", "scsi"],
          "exit_status": 0
        },
        "local_time": {
          "time_t": 1705569652,
          "asctime": "Thu Jan 18 10:20:52 2024 CET"
        },
        "device": {
          "name": "/dev/sda",
          "info_name": "/dev/sda",
          "type": "scsi",
          "protocol": "SCSI"
        },
        "scsi_vendor": "SAMSUNG",
        "scsi_product": "MZILT960HBHQ/007",
        "scsi_model_name": "SAMSUNG MZILT960HBHQ/007",
        "scsi_revision": "GXA0",
        "scsi_version": "SPC-5",
        "user_capacity": {
          "blocks": 1875385008,
          "bytes": 960197124096
        },
        "logical_block_size": 512,
        "physical_block_size": 4096,
        "scsi_lb_provisioning": {
          "name": "resource provisioned",
          "value": 1,
          "management_enabled": {
            "name": "LBPME",
            "value": 1
          },
          "read_zeros": {
            "name": "LBPRZ",
            "value": 1
          }
        },
        "rotation_rate": 0,
        "form_factor": {
          "scsi_value": 3,
          "name": "2.5 inches"
        },
        "logical_unit_id": "0x5002538b731302a0",
        "serial_number": "S5G1NC0W102239",
        "device_type": {
          "scsi_terminology": "Peripheral Device Type [PDT]",
          "scsi_value": 0,
          "name": "disk"
        },
        "scsi_transport_protocol": {
          "name": "SAS (SPL-4)",
          "value": 6
        },
        "smart_support": {
          "available": true,
          "enabled": true
        },
        "temperature_warning": {
          "enabled": true
        },
        "scsi_percentage_used_endurance_indicator": 0,
        "temperature": {
          "current": 35,
          "drive_trip": 74
        },
        "power_on_time": {
          "hours": 3862,
          "minutes": 30
        },
        "scsi_start_stop_cycle_counter": {
          "year_of_manufacture": "2023",
          "week_of_manufacture": "01",
          "accumulated_start_stop_cycles": 5,
          "specified_load_unload_count_over_device_lifetime": 0,
          "accumulated_load_unload_cycles": 0
        },
        "scsi_grown_defect_list": 0,
        "scsi_error_counter_log": {
          "read": {
            "errors_corrected_by_eccfast": 0,
            "errors_corrected_by_eccdelayed": 0,
            "errors_corrected_by_rereads_rewrites": 0,
            "total_errors_corrected": 0,
            "correction_algorithm_invocations": 0,
            "gigabytes_processed": "12.699",
            "total_uncorrected_errors": 0
          },
          "write": {
            "errors_corrected_by_eccfast": 0,
            "errors_corrected_by_eccdelayed": 0,
            "errors_corrected_by_rereads_rewrites": 0,
            "total_errors_corrected": 0,
            "correction_algorithm_invocations": 0,
            "gigabytes_processed": "6082.745",
            "total_uncorrected_errors": 0
          },
          "verify": {
            "errors_corrected_by_eccfast": 0,
            "errors_corrected_by_eccdelayed": 0,
            "errors_corrected_by_rereads_rewrites": 0,
            "total_errors_corrected": 0,
            "correction_algorithm_invocations": 0,
            "gigabytes_processed": "0.001",
            "total_uncorrected_errors": 0
          }
        },
        "scsi_pending_defects": {
          "count": 0
        },
        "scsi_self_test_0": {
          "code": {
            "value": 1,
            "string": "Background short"
          },
          "result": {
            "value": 0,
            "string": "Completed"
          },
          "power_on_time": {
            "hours": 5,
            "aka": "accumulated_power_on_hours"
          }
        },
        "scsi_extended_self_test_seconds": 3600
      }`)

	type fields struct {
		out []byte
		err error
	}

	type args struct {
		deviceName string
		deviceType string
	}

	tests := []struct {
		name    string
		fields  fields
		args    args
		want    *SmartCtlDeviceData
		wantErr bool
	}{
		{
			"+valid",
			fields{out: sampleValidSmartInfoScan},
			args{deviceName: "/dev/sda", deviceType: "raid,1,2,3"},
			&SmartCtlDeviceData{
				Device: &deviceParser{
					SerialNumber: "S5G1NC0W102239",
					Info: deviceInfo{
						Name:     "/dev/sda raid,1,2,3",
						InfoName: "/dev/sda",
						DevType:  "scsi",
						name:     "/dev/sda",
						raidType: "raid,1,2,3",
					},
					Smartctl:    smartctlField{Version: []int{7, 3}},
					SmartStatus: &smartStatus{SerialNumber: true},
				},
				Data: sampleValidSmartInfoScan,
			},
			false,
		},
		{
			"-executeErr",
			fields{out: sampleValidSmartInfoScan, err: errs.New("fail")},
			args{deviceName: "/dev/sda", deviceType: "raid,1,2,3"},
			nil,
			true,
		},
		{
			"-unmarshalErr",
			fields{out: []byte("{")},
			args{deviceName: "/dev/sda", deviceType: "raid,1,2,3"},
			nil,
			true,
		},
		{
			"-smartctlErr",
			fields{out: sampleFailedSmartInfoScan},
			args{deviceName: "/dev/sda", deviceType: "raid,1,2,3"},
			nil,
			true,
		},
		{
			"-missingSmartStatus",
			fields{out: sampleMissingSmartStatusSmartInfoScan},
			args{deviceName: "/dev/sda", deviceType: "raid,1,2,3"},
			nil,
			true,
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			m := mock.NewMockController(t)

			m.ExpectExecute().
				WithArgs("-a", "/dev/sda", "-d", "raid,1,2,3", "-j").
				WillReturnOutput(tt.fields.out).
				WillReturnError(tt.fields.err)

			got, err := getAllDeviceInfoByType(
				m,
				tt.args.deviceName,
				tt.args.deviceType,
			)
			if (err != nil) != tt.wantErr {
				t.Fatalf(
					"getAllDeviceInfoByType() error = %v, wantErr %v",
					err, tt.wantErr,
				)
			}

			if diff := cmp.Diff(
				tt.want, got,
				cmp.AllowUnexported(deviceParser{}, deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"getAllDeviceInfoByType() mismatch (-want +got):\n%s", diff,
				)
			}
		})
	}
}

//nolint:paralleltest,tparallel
func Test_getRaidDevices(t *testing.T) {
	log.DefaultLogger = stdlog.New(os.Stdout, "", stdlog.LstdFlags)

	type expectation struct {
		args []string
		err  error
		out  []byte
	}

	type args struct {
		deviceName string
		deviceType DeviceType
	}

	tests := []struct {
		name         string
		expectations []expectation
		args         args
		want         []*SmartCtlDeviceData
	}{
		{
			"+sat",
			[]expectation{
				{
					args: []string{"-a", "/dev/sda", "-d", "sat", "-j"},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d sat -j"),
				},
			},
			args{deviceName: "/dev/sda", deviceType: SAT},
			[]*SmartCtlDeviceData{
				{
					Device: &deviceParser{
						ModelName:    "INTEL SSDSC2BB120G6",
						SerialNumber: "PHWA619301M9120CGN",
						Info: deviceInfo{
							Name:     "/dev/sda sat",
							InfoName: "/dev/sda [SAT]",
							DevType:  "sat",
							name:     "/dev/sda",
							raidType: "sat",
						},
						Smartctl:    smartctlField{Version: []int{7, 3}},
						SmartStatus: &smartStatus{SerialNumber: true},
						SmartAttributes: smartAttributes{ //nolint:dupl
							Table: []table{
								{Attrname: "Reallocated_Sector_Ct", ID: 5},
								{Attrname: "Power_On_Hours", ID: 9},
								{Attrname: "Power_Cycle_Count", ID: 12},
								{
									Attrname: "Available_Reservd_Space",
									ID:       170,
									Thresh:   10,
								},
								{Attrname: "Program_Fail_Count", ID: 171},
								{Attrname: "Erase_Fail_Count", ID: 172},
								{Attrname: "Unsafe_Shutdown_Count", ID: 174},
								{
									Attrname: "Power_Loss_Cap_Test",
									ID:       175,
									Thresh:   10,
								},
								{Attrname: "SATA_Downshift_Count", ID: 183},
								{
									Attrname: "End-to-End_Error",
									ID:       184,
									Thresh:   90,
								},
								{Attrname: "Reported_Uncorrect", ID: 187},
								{Attrname: "Temperature_Case", ID: 190},
								{Attrname: "Unsafe_Shutdown_Count", ID: 192},
								{Attrname: "Temperature_Internal", ID: 194},
								{Attrname: "Current_Pending_Sector", ID: 197},
								{Attrname: "CRC_Error_Count", ID: 199},
								{Attrname: "Host_Writes_32MiB", ID: 225},
								{Attrname: "Workld_Media_Wear_Indic", ID: 226},
								{Attrname: "Workld_Host_Reads_Perc", ID: 227},
								{Attrname: "Workload_Minutes", ID: 228},
								{
									Attrname: "Available_Reservd_Space",
									ID:       232,
									Thresh:   10,
								},
								{Attrname: "Media_Wearout_Indicator", ID: 233},
								{Attrname: "Thermal_Throttle", ID: 234},
								{Attrname: "Host_Writes_32MiB", ID: 241},
								{Attrname: "Host_Reads_32MiB", ID: 242},
								{Attrname: "NAND_Writes_32MiB", ID: 243},
							},
						},
					},
					Data: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d sat -j"),
				},
			},
		},
		{
			"+scsi",
			[]expectation{
				{
					args: []string{"-a", "/dev/sda", "-d", "scsi", "-j"},
					out: mock.Outputs.Get("HBA_with_SAS_1").
						AllSmartInfoScans.Get("-a /dev/sda -d scsi -j"),
				},
			},
			args{deviceName: "/dev/sda", deviceType: SCSI},
			[]*SmartCtlDeviceData{
				{
					Device: &deviceParser{
						SerialNumber: "S5G1NC0W102239",
						Info: deviceInfo{
							Name:     "/dev/sda scsi",
							InfoName: "/dev/sda",
							DevType:  "scsi",
							name:     "/dev/sda",
							raidType: "scsi",
						},
						Smartctl:    smartctlField{Version: []int{7, 3}},
						SmartStatus: &smartStatus{SerialNumber: true},
					},
					Data: mock.Outputs.Get("HBA_with_SAS_1").
						AllSmartInfoScans.Get("-a /dev/sda -d scsi -j"),
				},
			},
		},
		{
			"-invalidType3ware",
			[]expectation{
				{
					args: []string{"-a", "/dev/sda", "-d", "3ware,0", "-j"},
					out: mock.Outputs.Get("env_1").
						AllSmartInfoScans.Get("-a /dev/sda -d 3ware,0 -j"),
				},
			},
			args{deviceName: "/dev/sda", deviceType: ThreeWare},
			nil,
		},
		// missing cases for:
		// - 3ware
		// - areca
		// - cciss
		// because of lack of test data
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			m := mock.NewMockController(t)

			for _, e := range tt.expectations {
				m.ExpectExecute().
					WithArgs(e.args...).
					WillReturnOutput(e.out).
					WillReturnError(e.err)
			}

			got := getRaidDevices(
				m, log.New(""), tt.args.deviceName, tt.args.deviceType,
			)
			if diff := cmp.Diff(
				tt.want, got,
				cmp.AllowUnexported(deviceParser{}, deviceInfo{}),
			); diff != "" {
				t.Fatalf("getRaidDevices() = %s", diff)
			}
		})
	}
}

func TestPlugin_getDevices(t *testing.T) {
	t.Parallel()

	type expect struct {
		raidScanExec bool
	}

	type fields struct {
		basicScanOut []byte
		basicScanErr error

		raidScanOut []byte
		raidScanErr error
	}

	tests := []struct {
		name         string
		expect       expect
		fields       fields
		wantBasic    []deviceInfo
		wantRaid     []deviceInfo
		wantMegaraid []deviceInfo
		wantErr      bool
	}{
		{
			"+env1",
			expect{true},
			fields{
				basicScanOut: mock.Outputs.Get("env_1").AllDevicesScan,
				raidScanOut:  mock.Outputs.Get("env_1").RaidDevicesScan,
			},
			[]deviceInfo{
				{
					Name:     "/dev/csmi0,0",
					InfoName: "/dev/csmi0,0",
					DevType:  "ata",
				},
				{
					Name:     "/dev/csmi0,2",
					InfoName: "/dev/csmi0,2",
					DevType:  "ata",
				},
				{
					Name:     "/dev/csmi0,3",
					InfoName: "/dev/csmi0,3",
					DevType:  "ata",
				},
				{
					Name:     "/dev/sdb",
					InfoName: "/dev/sdb",
					DevType:  "scsi",
				},
			},
			[]deviceInfo{
				{
					Name:     "/dev/sda",
					InfoName: "/dev/sda [SAT]",
					DevType:  "sat",
				},
			},
			nil,
			false,
		},
		{
			"+envMac",
			expect{true},
			fields{
				basicScanOut: mock.OutputEnvMacScanBasic,
				raidScanOut:  mock.OutputEnvMacScanRaid,
			},
			[]deviceInfo{
				{
					Name:     "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
					InfoName: "IOService:/AppleARMPE/arm-io@10F00000/AppleT811xIO/ans@77400000/AppleASCWrapV4/iop-ans-nub/RTBuddy(ANS2)/RTBuddyService/AppleANS3NVMeController/NS_01@1", //nolint:lll
					DevType:  "nvme",
				},
			},
			nil,
			nil,
			false,
		},
		{
			"+HBA_with_SAS_1",
			expect{true},
			fields{
				basicScanOut: mock.Outputs.Get("HBA_with_SAS_1").AllDevicesScan,
				raidScanOut: mock.Outputs.Get(
					"HBA_with_SAS_1",
				).RaidDevicesScan,
			},
			nil,
			[]deviceInfo{
				{Name: "/dev/sda", InfoName: "/dev/sda", DevType: "scsi"},
				{Name: "/dev/sdaa", InfoName: "/dev/sdaa", DevType: "scsi"},
				{Name: "/dev/sdab", InfoName: "/dev/sdab", DevType: "scsi"},
				{Name: "/dev/sdac", InfoName: "/dev/sdac", DevType: "scsi"},
				{Name: "/dev/sdad", InfoName: "/dev/sdad", DevType: "scsi"},
				{Name: "/dev/sdae", InfoName: "/dev/sdae", DevType: "scsi"},
				{Name: "/dev/sdaf", InfoName: "/dev/sdaf", DevType: "scsi"},
				{Name: "/dev/sdb", InfoName: "/dev/sdb", DevType: "scsi"},
				{Name: "/dev/sdc", InfoName: "/dev/sdc", DevType: "scsi"},
				{Name: "/dev/sdd", InfoName: "/dev/sdd", DevType: "scsi"},
				{Name: "/dev/sde", InfoName: "/dev/sde", DevType: "scsi"},
				{Name: "/dev/sdf", InfoName: "/dev/sdf", DevType: "scsi"},
				{Name: "/dev/sdg", InfoName: "/dev/sdg", DevType: "scsi"},
				{Name: "/dev/sdh", InfoName: "/dev/sdh", DevType: "scsi"},
				{Name: "/dev/sdi", InfoName: "/dev/sdi", DevType: "scsi"},
				{Name: "/dev/sdj", InfoName: "/dev/sdj", DevType: "scsi"},
				{Name: "/dev/sdk", InfoName: "/dev/sdk", DevType: "scsi"},
				{Name: "/dev/sdl", InfoName: "/dev/sdl", DevType: "scsi"},
				{Name: "/dev/sdm", InfoName: "/dev/sdm", DevType: "scsi"},
				{Name: "/dev/sdn", InfoName: "/dev/sdn", DevType: "scsi"},
				{Name: "/dev/sdo", InfoName: "/dev/sdo", DevType: "scsi"},
				{Name: "/dev/sdp", InfoName: "/dev/sdp", DevType: "scsi"},
				{Name: "/dev/sdq", InfoName: "/dev/sdq", DevType: "scsi"},
				{Name: "/dev/sdr", InfoName: "/dev/sdr", DevType: "scsi"},
				{Name: "/dev/sds", InfoName: "/dev/sds", DevType: "scsi"},
				{Name: "/dev/sdt", InfoName: "/dev/sdt", DevType: "scsi"},
				{Name: "/dev/sdu", InfoName: "/dev/sdu", DevType: "scsi"},
				{Name: "/dev/sdv", InfoName: "/dev/sdv", DevType: "scsi"},
				{Name: "/dev/sdw", InfoName: "/dev/sdw", DevType: "scsi"},
				{Name: "/dev/sdx", InfoName: "/dev/sdx", DevType: "scsi"},
				{Name: "/dev/sdy", InfoName: "/dev/sdy", DevType: "scsi"},
				{Name: "/dev/sdz", InfoName: "/dev/sdz", DevType: "scsi"},
			},
			nil,
			false,
		},
		{
			"-basicScanErr",
			expect{false},
			fields{
				basicScanOut: mock.OutputScan,
				basicScanErr: errors.New("fail"),
			},
			nil,
			nil,
			nil,
			true,
		},
		{
			"-raidScanErr",
			expect{true},
			fields{
				basicScanOut: mock.OutputScan,
				raidScanOut:  mock.OutputScanTypeSAT,
				raidScanErr:  errors.New("fail"),
			},
			nil,
			nil,
			nil,
			true,
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			m := &mock.MockController{}

			m.ExpectExecute().
				WithArgs("--scan", "-j").
				WillReturnOutput(tt.fields.basicScanOut).
				WillReturnError(tt.fields.basicScanErr)

			if tt.expect.raidScanExec {
				m.ExpectExecute().
					WithArgs("--scan", "-d", "sat", "-j").
					WillReturnOutput(tt.fields.raidScanOut).
					WillReturnError(tt.fields.raidScanErr)
			}

			p := &Plugin{ctl: m}

			gotBasic, gotRaid, gotMegaraid, err := p.getDevices()
			if (err != nil) != tt.wantErr {
				t.Fatalf(
					"Plugin.getDevices() error = %v, wantErr %v",
					err,
					tt.wantErr,
				)
			}

			if diff := cmp.Diff(
				tt.wantBasic, gotBasic,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"Plugin.getDevices() Basic devices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if diff := cmp.Diff(
				tt.wantRaid, gotRaid,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"Plugin.getDevices() Raid devices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if diff := cmp.Diff(
				tt.wantMegaraid, gotMegaraid,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf(
					"Plugin.getDevices() MegaRaid devices mismatch (-want +got):\n%s",
					diff,
				)
			}

			if err := m.ExpectationsWhereMet(); err != nil {
				t.Fatalf(
					"Plugin.getDevices() expectations where not met, error = %v",
					err,
				)
			}
		})
	}
}

func Test_formatDeviceOutput(t *testing.T) {
	t.Parallel()

	sampleBasicDev1 := deviceInfo{
		Name:     "/dev/csmi0,0",
		InfoName: "/dev/csmi0,0",
		DevType:  "ata",
	}

	sampleBasicDev2 := deviceInfo{
		Name:     "/dev/csmi0,2",
		InfoName: "/dev/csmi0,2",
		DevType:  "ata",
	}

	sampleRaidDev1 := deviceInfo{
		Name:     "/dev/sda",
		InfoName: "/dev/sda [SAT]",
		DevType:  "sat",
	}

	sampleRaidDev2 := deviceInfo{
		Name:     "/dev/sdb",
		InfoName: "/dev/sdb [SAT]",
		DevType:  "sat",
	}

	sampleMegaraidDev1 := deviceInfo{
		Name:     "frogs_hallucination",
		InfoName: "frogs_hallucination",
		DevType:  "megaraid",
	}

	sampleMegaraidDev2 := deviceInfo{
		Name:     "cows_imagination",
		InfoName: "cows_imagination",
		DevType:  "megaraid",
	}

	type args struct {
		basic []deviceInfo
		raid  []deviceInfo
	}

	tests := []struct {
		name            string
		args            args
		wantBasicDev    []deviceInfo
		wantRaidDev     []deviceInfo
		wantMegaraidDev []deviceInfo
	}{
		{
			"+valid",
			args{
				[]deviceInfo{sampleBasicDev1, sampleBasicDev2},
				[]deviceInfo{
					sampleRaidDev1, sampleRaidDev2,
					sampleMegaraidDev1, sampleMegaraidDev2,
				},
			},
			[]deviceInfo{sampleBasicDev1, sampleBasicDev2},
			[]deviceInfo{sampleRaidDev1, sampleRaidDev2},
			[]deviceInfo{sampleMegaraidDev1, sampleMegaraidDev2},
		},
		{
			"+megaraidDevices",
			args{
				[]deviceInfo{},
				[]deviceInfo{
					sampleRaidDev1, sampleRaidDev2,
					sampleMegaraidDev1, sampleMegaraidDev2,
				},
			},
			nil,
			[]deviceInfo{sampleRaidDev1, sampleRaidDev2},
			[]deviceInfo{sampleMegaraidDev1, sampleMegaraidDev2},
		},
		{
			"-duplicateDevInRaidAndBasic",
			args{
				[]deviceInfo{sampleBasicDev1, sampleRaidDev1},
				[]deviceInfo{sampleRaidDev1, sampleRaidDev2},
			},
			[]deviceInfo{sampleBasicDev1},
			[]deviceInfo{sampleRaidDev1, sampleRaidDev2},
			nil,
		},
		{
			"-noDevices",
			args{[]deviceInfo{}, []deviceInfo{}},
			nil,
			nil,
			nil,
		},
		{
			"-nilDevices",
			args{nil, nil},
			nil,
			nil,
			nil,
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			gotBasicDev, gotRaidDev, gotMegaraidDev := formatDeviceOutput(
				tt.args.basic, tt.args.raid,
			)

			if diff := cmp.Diff(
				tt.wantBasicDev, gotBasicDev,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf("formatDeviceOutput() gotBasicDev = %s", diff)
			}

			if diff := cmp.Diff(
				tt.wantRaidDev, gotRaidDev,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf("formatDeviceOutput() gotRaidDev = %s", diff)
			}

			if diff := cmp.Diff(
				tt.wantMegaraidDev, gotMegaraidDev,
				cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf("formatDeviceOutput() gotMegaraidDev = %s", diff)
			}
		})
	}
}

func TestPlugin_scanDevices(t *testing.T) {
	t.Parallel()

	type fields struct {
		execErr error
		execOut []byte
	}

	type args struct {
		args []string
	}

	tests := []struct {
		name    string
		fields  fields
		args    args
		want    []deviceInfo
		wantErr bool
	}{
		{
			"+valid",
			fields{execOut: mock.OutputScan},
			args{[]string{"--scan", "-j"}},
			[]deviceInfo{
				{
					Name:     "/dev/csmi0,0",
					InfoName: "/dev/csmi0,0",
					DevType:  "ata",
				},
				{
					Name:     "/dev/csmi0,2",
					InfoName: "/dev/csmi0,2",
					DevType:  "ata",
				},
				{
					Name:     "/dev/csmi0,3",
					InfoName: "/dev/csmi0,3",
					DevType:  "ata",
				},
			},
			false,
		},
		{
			"-execErr",
			fields{execOut: mock.OutputScan, execErr: errors.New("fail")},
			args{[]string{"--scan", "-j"}},
			nil,
			true,
		},
		{
			"-marshalErr",
			fields{execOut: []byte("{")},
			args{[]string{"--scan", "-j"}},
			nil,
			true,
		},
	}
	for _, tt := range tests {
		tt := tt
		t.Run(tt.name, func(t *testing.T) {
			t.Parallel()

			m := &mock.MockController{}

			m.ExpectExecute().
				WithArgs(tt.args.args...).
				WillReturnOutput(tt.fields.execOut).
				WillReturnError(tt.fields.execErr)

			p := &Plugin{ctl: m}

			got, err := p.scanDevices(tt.args.args...)
			if (err != nil) != tt.wantErr {
				t.Fatalf(
					"Plugin.scanDevices() error = %v, wantErr %v",
					err, tt.wantErr,
				)
			}

			if diff := cmp.Diff(
				tt.want, got, cmp.AllowUnexported(deviceInfo{}),
			); diff != "" {
				t.Fatalf("Plugin.scanDevices() = %s", diff)
			}

			if err := m.ExpectationsWhereMet(); err != nil {
				t.Fatalf(
					"Plugin.scanDevices() expectations where not met, error = %v",
					err,
				)
			}
		})
	}
}
