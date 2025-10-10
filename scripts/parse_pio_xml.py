import xml.etree.ElementTree as ET
import sys

try:
    tree = ET.parse('pio-check-report.xml')
    root = tree.getroot()
    summary = ['# PlatformIO Static Analysis Report\n']
    defect_count = 0
    for file in root.findall('.//file'):
        filename = file.get('name')
        for error in file.findall('error'):
            defect_count += 1
            severity = error.get('severity', 'unknown').capitalize()
            message = error.get('msg', 'No message')
            line = error.get('line', '?')
            summary.append(f'- **{severity}** in `{filename}` at line {line}: {message}')
    if defect_count == 0:
        summary.append('✅ No issues found!')
    with open('pio-check-summary.md', 'w') as f:
        f.write('\n'.join(summary))
except Exception as e:
    with open('pio-check-summary.md', 'w') as f:
        f.write(f"# PlatformIO Static Analysis Report\n❌ Could not parse XML report: {str(e)}")

