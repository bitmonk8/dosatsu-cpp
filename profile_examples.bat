del /S /Q artifacts\profile
python Examples\run_examples.py --profile
python scripts/analyze_profile.py --directory artifacts/profile
