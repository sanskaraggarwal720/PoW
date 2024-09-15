import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from xgboost import XGBClassifier
from fastapi import FastAPI
from fastapi.responses import FileResponse

# Define the Fastapi app
app = FastAPI()


xgb_model = XGBClassifier(
    scale_pos_weight=1000,
    n_estimators = 500, 
    verbosity = 0, 
    learning_rate = 0.1, 
    random_state = 7, 
    early_stopping_rounds = 10
)

# Mapping functions to handle categorical data
# Mapping functtions with number instead of one-hot-enoconding
browser_map = {
    'Safari': 0.0, 
    'Chrome': 1.0, 
    'Opera': 2.0, 
    'Firefox': 3.0, 
    'Edge': 4.0, 
    'NOT-FOUND': 5.0
}

method_map = {
    'GET': 0.0, 
    'POST': 1.0,
    'HEAD': 2.0, 
    'OPTIONS': 3.0
}

status_code_map = {
    "200":0.0,
    "301":1.0,
    "400":2.0, 
    "403":3.0, 
    "404":4.0, 
    "500":5.0, 
    "502":6.0
}

ddos_map = {
    'NoDDoS': 0.0,
    'DDoS': 1.0
}

# paths
file_path = "../csv_files/ddos_data.csv"
logger_path = "../csv_files/logger.csv"
complete_logs_path = "../csv_files/complete_logs.csv"
blocked_path = "../csv_files/blocked.csv"


@app.get("/")
def read():
    return {"Hello":"World"}

# Load CSV data and train the model
@app.get("/start-training") 
async def run_tree_ensemble():

    try:
        # Load CSV using pandas
        df = pd.read_csv(file_path)
        df1 = df[['Browser' ,'Request Count', 'HTTP Method', 'Status Code','Response Time (ms)']]
        df1['Browser'] = df1['Browser'].map(browser_map)
        df1['HTTP Method'] = df1['HTTP Method'].map(method_map)
        df1['Status Code'] = df1['Status Code'].map(status_code_map)
        X = df1
        Y = df['DDoS Happening']
        y = Y.apply(lambda x: 1 if x == 'DDOS' else 0)
        X_train, X_, y_train, y_ = train_test_split(X, y, test_size=0.25, random_state=42, shuffle=True)
        X_test, X_eval, y_test, y_eval = train_test_split(X_, y_, test_size=0.5, random_state=42, shuffle=True)
        xgb_model.fit(X_train,y_train, eval_set = [(X_eval,y_eval)])
        y1_pred_tree = xgb_model.predict(X_train)
        y2_pred_tree = xgb_model.predict(X_eval)
        y3_pred_tree = xgb_model.predict(X_test)
        acc1 = np.mean(y_train==y1_pred_tree)
        acc2 = np.mean(y_eval==y2_pred_tree)
        acc3 = np.mean(y_test==y3_pred_tree)
        
        print("Training Score: ",acc1*100)
        print("Eval-Set Score: ",acc2*100)
        print("Test-Set Score",acc3*100)

        return {"Training Score":acc1*100, "Eval-Set Score":acc2*100, "Test-Set Score":acc3*100}
        
    except Exception as e:

        print(f"Error: {e}")
        return {"Error Occured in Training"}


@app.get("/predict")
async def prediction():
    try:
        data=pd.read_csv(logger_path)
        data.to_csv(complete_logs_path, mode='a', header=False, index=False)
        temp_array=np.empty((data.shape[0],5))
        for i in range(data.shape[0]):
            temp_array[i][0] = browser_map[data.loc[i]["BROWSER"]]   # browser
            temp_array[i][1] = data.loc[i]["REQUEST-COUNT"]              # request-count
            temp_array[i][2] = method_map[data.loc[i]["METHOD"]]  # http-method
            temp_array[i][3] = status_code_map[str(data.loc[i]["STATUS-CODE"])]  # status-code
            temp_array[i][4] = data.loc[i]["RESPONSE-TIME"]              # response-time

        y_pred = xgb_model.predict(temp_array)
        block=[]
        for i,y in enumerate(y_pred):
            if y == 1:
                temp_dict = {
                    "IP": data.loc[i]["IP"],
                    "TIME-STAMP": data.loc[i]["TIME-STAMP"]
                }
                block.append(temp_dict)
        
        pd.DataFrame(block).to_csv(blocked_path,index=False,header=False,mode='a')
 
        empty_df = pd.DataFrame(columns=data.columns)

       # Write the empty DataFrame to the CSV file, keeping only the header
        empty_df.to_csv(logger_path, index=False)
        
        return "Prediction Done"
    
    except Exception as e:
        print(f"Error: {e}")
        return {"Error Occured in Prediction"}
    
@app.get("/viz")
async def get_viz():
    try:
        return FileResponse('static/index.html')
    
    except Exception as e:
        print(f"Error: {e}")
        return {"Error Occured"}
            
