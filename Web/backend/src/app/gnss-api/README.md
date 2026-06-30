# Backend Database to store the data 

# To run, need Python 
// My configuration on the laptop:
source .venv/bin/activate.fish

or 

source .venv/bin/activate

install uvicorn: pip install fastapi "uvicorn[standard]" 

run it:
uvicorn main:app --host 0.0.0.0 --port 8000 --reload


