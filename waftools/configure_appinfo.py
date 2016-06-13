import json

from waflib.Configure import conf


@conf
def configure_appinfo(ctx, transforms):
    with open('package.json', 'r') as appinfo_file:
        appinfo_json = json.load(appinfo_file)

    for transform in transforms:
        transform(appinfo_json)

    with open('package.json', 'w') as appinfo_file:
        json.dump(appinfo_json, appinfo_file, indent=2, sort_keys=True, separators=(',', ': '))
