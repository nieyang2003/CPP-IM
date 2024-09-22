package snowflake

import (
	"fmt"
	"time"

	"github.com/sony/sonyflake"
)

var flake *sonyflake.Sonyflake
var machineID uint16

func GetMachineID() (uint16, error) {
	return machineID, nil
}

func Init(machine_id uint16) (err error) {
	machineID = machine_id
	t, _ := time.Parse("2006-01-02", "2024-01-01")
	// 将时间、函数绑定到setting中
	settings := sonyflake.Settings{
		StartTime: t,
		MachineID: GetMachineID,
	}
	flake = sonyflake.NewSonyflake(settings)
	return
}

func GetID() (id uint64, err error) {
	if flake == nil {
		err = fmt.Errorf("snoy flake not inited")
		return
	}

	id, err = flake.NextID()
	return
}
