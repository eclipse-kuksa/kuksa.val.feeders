# VSS Mappings

The DDS Provider has a dependency to [VSS](https://github.com/COVESA/vehicle_signal_specification).
The signals sent to Databroker must be defined in the Databroker. The file `mapping.yml` specifies which VSS signals
DDS provider actually use. The file `vss.json`is a copy of the "official" VSS JSOn release artifact
for the selected version.

## How to update VSS version

Check `mapping.yml` in  the version indicated by the symbolic link `latest`, and compare the signals specified
there with the latest VSS version. If there are no changes there is no strict reason to update, but could be good
anyway to verify that no regressions are introduced.

To update, create a new folder and add copy the JSON artifact from the official
[VSS release](https://github.com/COVESA/vehicle_signal_specification/releases). Change the `latest`symbolic link.
The JSON file is present as it is used when starting Databroker for integration tests.
Copy `mapping.yml` and check if there are any introduced changes to VSS related to the specified signals

Update [generate_py_dataclass.sh](https://github.com/eclipse/kuksa.val.feeders/blob/main/dds2val/ddsproviderlib/idls/generate_py_dataclass.sh)
so that it refers to new tags for VSS and VSS-tools. Look for the version after `-b` below.

```
git clone https://github.com/COVESA/vehicle_signal_specification.git -b v4.0rc1
git clone https://github.com/COVESA/vss-tools.git -b v4.0rc2
```

### If VSS signals in `mapping.yml` has changed

Then you need to do a search in all files and update names accordingly.
