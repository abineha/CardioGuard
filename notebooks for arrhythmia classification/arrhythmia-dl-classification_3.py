import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler, LabelEncoder
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv1D, MaxPooling1D, Dense, Flatten, Dropout
from tensorflow.keras.utils import to_categorical
import matplotlib.pyplot as plt
import seaborn as sns
from wfdb import rdsamp, rdann
import os

# Cell 1: Data Loading and Preprocessing
def load_and_preprocess_data(record_names, sequence_length=200):
    features = []
    labels = []
    
    for record in record_names:
        # Load signal data
        signal, fields = rdsamp(f'mitdb/{record}')
        # Load annotations
        ann = rdann(f'mitdb/{record}', 'atr')
        
        # Extract sequences for each beat
        for idx in range(len(ann.sample)-1):
            start = ann.sample[idx]
            end = ann.sample[idx+1]
            
            if end - start > sequence_length:
                # Extract fixed-length sequence
                beat = signal[start:start+sequence_length, 0]
                features.append(beat)
                labels.append(ann.symbol[idx])
    
    # Convert to numpy arrays
    features = np.array(features)
    labels = np.array(labels)
    
    # Normalize features
    scaler = StandardScaler()
    features_reshaped = features.reshape(-1, features.shape[-1])
    features_scaled = scaler.fit_transform(features_reshaped)
    features = features_scaled.reshape(features.shape)
    
    # Encode labels
    label_encoder = LabelEncoder()
    labels_encoded = label_encoder.fit_transform(labels)
    labels_onehot = to_categorical(labels_encoded)
    
    return features, labels_onehot, label_encoder

# Cell 2: Model Definition
def create_model(sequence_length, n_classes):
    model = Sequential([
        Conv1D(32, kernel_size=5, activation='relu', input_shape=(sequence_length, 1)),
        MaxPooling1D(pool_size=2),
        Conv1D(64, kernel_size=5, activation='relu'),
        MaxPooling1D(pool_size=2),
        Conv1D(64, kernel_size=5, activation='relu'),
        MaxPooling1D(pool_size=2),
        Flatten(),
        Dense(64, activation='relu'),
        Dropout(0.5),
        Dense(n_classes, activation='softmax')
    ])
    
    model.compile(optimizer='adam',
                 loss='categorical_crossentropy',
                 metrics=['accuracy'])
    
    return model

# Cell 3: Training and Evaluation
def train_and_evaluate_model():
    # Load data
    record_names = ['100', '101', '102', '103', '104']  # Add more records as needed
    sequence_length = 200
    X, y, label_encoder = load_and_preprocess_data(record_names, sequence_length)
    
    # Reshape for CNN input
    X = X.reshape(X.shape[0], X.shape[1], 1)
    
    # Split data
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    
    # Create and train model
    model = create_model(sequence_length, y.shape[1])
    history = model.fit(X_train, y_train,
                       epochs=20,
                       batch_size=32,
                       validation_split=0.2,
                       verbose=1)
    
    # Evaluate model
    y_pred = model.predict(X_test)
    y_pred_classes = np.argmax(y_pred, axis=1)
    y_test_classes = np.argmax(y_test, axis=1)
    
    # Convert back to original labels
    y_pred_labels = label_encoder.inverse_transform(y_pred_classes)
    y_test_labels = label_encoder.inverse_transform(y_test_classes)
    
    return history, y_test_labels, y_pred_labels

# Cell 4: Visualization
def plot_metrics(history, y_test, y_pred):
    # Plot training history
    plt.figure(figsize=(12, 4))
    
    plt.subplot(1, 2, 1)
    plt.plot(history.history['accuracy'], label='Training Accuracy')
    plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
    plt.title('Model Accuracy')
    plt.xlabel('Epoch')
    plt.ylabel('Accuracy')
    plt.legend()
    
    plt.subplot(1, 2, 2)
    plt.plot(history.history['loss'], label='Training Loss')
    plt.plot(history.history['val_loss'], label='Validation Loss')
    plt.title('Model Loss')
    plt.xlabel('Epoch')
    plt.ylabel('Loss')
    plt.legend()
    
    plt.tight_layout()
    plt.show()
    
    # Plot confusion matrix
    conf_matrix = confusion_matrix(y_test, y_pred)
    plt.figure(figsize=(10, 8))
    sns.heatmap(conf_matrix, annot=True, fmt='d', cmap='Blues')
    plt.title('Confusion Matrix')
    plt.ylabel('True Label')
    plt.xlabel('Predicted Label')
    plt.show()

# Cell 5: Main Execution
if __name__ == "__main__":
    history, y_test, y_pred = train_and_evaluate_model()
    
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred))
    
    plot_metrics(history, y_test, y_pred)
