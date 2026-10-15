import sys
import json
import dialog


vars:dict[str,str] = { }

def export_to_makefile_args() -> str:
        out:str = ""
        for k,v in vars:
                out += f"{k}={v} \n"
        return out
def import_variables(cfgp:str):
        f = open(cfgp,"rc")
        cfg:str = f.readlines()
        for line in cfg:
                pairs = line.split("=")
                key = pairs[0].strip()
                value = pairs[1].strip()
                vars[key] = value

