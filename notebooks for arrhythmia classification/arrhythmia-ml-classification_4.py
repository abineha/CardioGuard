import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.svm import SVC
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
import matplotlib.pyplot as plt
import seaborn as sns
from wfdb import rdsamp, rdann
import os

# Cell 1: Data Loading and Preprocessing
def load_and_preprocess_data(record_names):
    features = []
    labels = []
    
    for record in record_names:
        # Load signal data
        signal, fields = rdsamp(f'mitdb/{record}')
        # Load annotations
        ann = rdann(f'mitdb/{record}', 'atr')
        
        # Extract features for each beat
        for idx in range(len(ann.sample)-1):
            start = ann.sample[idx]
            end = ann.sample[idx+1]
            
            if end - start > 50:  # Ensure minimum beat length
                # Extract RR intervals and signal features
                beat = signal[start:end, 0]  # Channel 1
                
                # Basic features
                features.append([
                    np.mean(beat),
                    np.std(beat),
                    np.max(beat),
                    np.min(beat),
                    end - start,  # RR interval
                    np.median(beat),
                    np.percentile(beat, 25),
                    np.percentile(beat, 75)
                ])
                labels.append(ann.symbol[idx])
    
    return np.array(features), np.array(labels)

# Cell 2: Model Training and Evaluation
def train_and_evaluate_model():
    # Load data
    record_names = ['100', '101', '102', '103', '104']  # Add more records as needed
    X, y = load_and_preprocess_data(record_names)
    
    # Split data
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    
    # Scale features
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    
    # Train SVM model
    svm = SVC(kernel='rbf', random_state=42)
    svm.fit(X_train_scaled, y_train)
    
    # Make predictions
    y_pred = svm.predict(X_test_scaled)
    
    # Calculate metrics
    accuracy = accuracy_score(y_test, y_pred)
    report = classification_report(y_test, y_pred)
    conf_matrix = confusion_matrix(y_test, y_pred)
    
    return accuracy, report, conf_matrix, y_test, y_pred

# Cell 3: Visualization
def plot_metrics(conf_matrix, y_test, y_pred):
    # Plot confusion matrix
    plt.figure(figsize=(10, 8))
    sns.heatmap(conf_matrix, annot=True, fmt='d', cmap='Blues')
    plt.title('Confusion Matrix')
    plt.ylabel('True Label')
    plt.xlabel('Predicted Label')
    plt.show()
    
    # Plot distribution of predictions
    plt.figure(figsize=(10, 6))
    pd.Series(y_pred).value_counts().plot(kind='bar')
    plt.title('Distribution of Predicted Classes')
    plt.xlabel('Class')
    plt.ylabel('Count')
    plt.show()

# Cell 4: Main Execution
if __name__ == "__main__":
    accuracy, report, conf_matrix, y_test, y_pred = train_and_evaluate_model()
    
    print(f"Accuracy: {accuracy:.4f}")
    print("\nClassification Report:")
    print(report)
    
    plot_metrics(conf_matrix, y_test, y_pred)
















