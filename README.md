
<div align="center">

#  Cinephile

</div>

<div align="center">

[![Qt](https://img.shields.io/badge/Qt-6.2+-41CD52?style=for-the-badge&logo=qt&logoColor=white)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

**Your personal movie universe. Find, track, and stream anything**

*Discover, organize, and explore the world of cinema with AI-powered recommendations*

[📥 Download Latest Release](https://github.com/Najaf-Ali-Imran/Cinephile/releases) • [🐛 Report Bug](https://github.com/Najaf-Ali-Imran/Cinephile/issues) • [💡 Feature Request](https://github.com/Najaf-Ali-Imran/Cinephile/issues)

</div>

---

## ✨ Overview

Cinephile is a sophisticated, feature-rich desktop application that acts as your all-in-one cinematic command center. Built with modern C++ and Qt 6, it seamlessly combines advanced movie discovery, secure personal library management, and instant streaming links into a single, powerful platform.

This isn't just another movie browser. Through a secure user account system (supporting both Email/Password and Google Sign-In), Cinephile becomes your personal cinema companion. It allows you to meticulously manage your viewing experience with detailed libraries—including your Favorites, Watched History, and unlimited user-created Custom Lists.

> **🎯 Perfect for:** Movie buffs, TV series enthusiasts, and anyone who loves discovering, organizing, and watching content in a unified experience.

---

## 🚀 Key Features

### 🎬 **Comprehensive Discovery & Instant Watching**
- **Dynamic Home Dashboard:** Explore curated carousels for Trending, Now Playing, Popular, and Top Rated content.
- **Advanced Filtering Engine:** Pinpoint movies with 10+ filters, including genre, release year, vote average, and runtime.
- **Direct Streaming Links:** Go from discovery to watching instantly with integrated links to stream content.
- **Detailed Information Pages:** Access full cast lists, watch official trailers, and browse recommendations for any title.

### 📚 **Personal Library & Profile Management**
- **Secure User Authentication:** Safe sign-up and login via Email/Password or Google OAuth 2.0.
- **Cloud-Backed Library:** Your collections are saved to your account and accessible from any installation.
- **Pre-defined Libraries:** Automatically manage your **Watchlist**, **Favorites**, and **Watched History**.
- **Unlimited Custom Lists:** Create, name, and manage your own personalized collections for any mood or genre.
- **Profile Customization:** Change your display name and upload a custom profile picture.

### 🤖 **AI-Powered Cinema Assistant**
- **CineAI Chatbot:** A witty, knowledgeable movie expert powered by the Google Gemini API.
- **Personalized Recommendations:** The app analyzes your viewing history and favorites to suggest titles you'll love.
- **Intelligent Taste Analysis:** Understands your preferences by scoring genres and keywords from your library.
- **Natural Language Queries:** Ask for recommendations or movie trivia in plain English.

### 🎨 **Modern Native Experience**
- **Sleek Cinematic Theme:** A dark, immersive UI optimized for movie browsing.
- **Native Desktop Performance:** Built with C++/Qt for a fast, responsive, and resource-efficient experience.
- **Custom Frameless Window:** A modern look with custom-drawn window controls.
- **Skeleton Loading Animations:** A professional and smooth loading experience while data is fetched from the network.
---

## 📸 Screenshots

<div align="center">

### 🏠 Home Dashboard

<img src="/screenshots/poster.png" alt="Home Dashboard" width="100%">

### 🔍 Login/Signup

<img src="/screenshots/Login.png" alt="Advanced Filtering" width="100%">

### 📖 Watch Movies

<img src="/screenshots/Watch.png" alt="Movie Details" width="100%">

### 📚 Recomendations/AI

<img src="/screenshots/Ai.png" alt="Personal Library" width="100%">


</div>

---

## 🛠️ Tech Stack

<div align="center">

| Category | Technologies |
|----------|-------------|
| **Core Framework** | Qt 6.2+ (C++17) |
| **Modules** | QtWidgets, QtNetwork, QtSvg, QtCore |
| **Build System** | CMake 3.16+ |
| **Movie Data** | The Movie Database (TMDb) |
| **Authentication** | Firebase Authentication |
| **Database** | Firestore Database |
| **AI Assistant** | Google Gemini  |

</div>

---

## 🏗️ Architecture

Cinephile follows modern C++ design patterns and Qt best practices:

- **🏛️ Singleton Pattern** - UserManager & ConfigManager for global state
- **🔄 MVC-like Architecture** - Clear separation of concerns
- **⚡ Asynchronous Operations** - Non-blocking network requests
- **📡 Signal-Slot Communication** - Reactive UI updates
- **🔐 Secure Configuration** - External API key management

### Key Components

```
📁 Cinephile/
├── 🔐 ConfigManager      # API key & settings management
├── 👤 UserManager        # User state & authentication
├── 🏠 MainWindow         # Main application window
├── 📊 DashboardWidget    # Page navigation & skeleton loading
├── 🎬 MovieCard          # Reusable movie display component
├── 📖 MovieDetailWidget  # Detailed movie information
├── 📚 LibraryPageWidget  # Personal library management
├── 🔍 CategoryFilterWidget # Advanced search & filtering
├── 🤖 ChatbotWidget      # AI assistant interface
└── 🌐 FirestoreService   # Cloud database operations
```

---

## 📦 Installation

### For Users (Recommended)

**🎯 Quick Start:** Download the latest pre-built version for your operating system:

<div align="center">

[![Download for Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://github.com/Najaf-Ali-Imran/Cinephile/releases)


</div>

### For Developers

Want to build from source or contribute? Follow the development setup below.

---

## 🔧 Development Setup

### Prerequisites

Ensure you have the following installed:

- **Qt 6 SDK** (6.2 or later) - [Download here](https://www.qt.io/download)
- **C++ Compiler** with C++17 support (GCC, Clang, or MSVC)
- **CMake** (3.16 or later) - [Download here](https://cmake.org/download/)

### 🔑 API Configuration (Critical Step)

> ⚠️ **Important:** The application requires API keys to function. These are not included for security reasons.

1. **Locate the template file:**
   ```bash
   config.template.json
   ```

2. **Create your configuration:**
   ```bash
   cp config.template.json config.json
   ```

3. **Obtain API keys from these services:**
   - 🎬 [The Movie Database (TMDb)](https://www.themoviedb.org/settings/api)
   - 🔐 [Google Cloud Console](https://console.cloud.google.com/) (Enable Firebase, OAuth, Gemini)

4. **Fill in your `config.json`:**
   ```json
   {
     "Firebase": {
       "ApiKey": "YOUR_FIREBASE_API_KEY_HERE"
     },
     "GoogleOAuth": {
       "ClientId": "YOUR_GOOGLE_OAUTH_CLIENT_ID_HERE",
       "ClientSecret": "YOUR_GOOGLE_OAUTH_CLIENT_SECRET_HERE"
     },
     "Gemini": {
       "ApiKey": "YOUR_GEMINI_API_KEY_HERE"
     }
   }
   ```

### 🏗️ Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/Najaf-Ali-Imran/Cinephile.git
   cd Cinephile
   ```

2. **Create build directory:**
   ```bash
   mkdir build && cd build
   ```

3. **Configure with CMake:**
   ```bash
   cmake ..
   ```

4. **Build the project:**
   ```bash
   cmake --build .
   ```

5. **Copy configuration:**
   ```bash
   cp ../config.json . 
   ```

6. **Run Cinephile:**
   ```bash
   ./Cinephile  # Linux/macOS
   Cinephile.exe  # Windows
   ```

---

## 🔒 Security & Privacy

Your security and privacy are our top priorities:

### 🛡️ Security Measures
- **🔐 No Hardcoded Secrets** - All API keys loaded from local config file
- **🔒 Secure Authentication** - Google Firebase handles all password management
- **📊 Encrypted Data Transit** - All API communications use HTTPS
- **🚫 Zero Tracking** - No user analytics or behavioral tracking

### 🗂️ Data Management
- **☁️ Cloud Sync** - Your lists sync securely across devices via Firestore
- **👤 Account Control** - Full control over your data and account deletion
- **🔗 External Links** - "Watch Now" links to third-party sites (proceed at your own discretion)

### 📋 What We Store
- ✅ Account information (email, display name)
- ✅ Your movie lists and preferences
- ✅ Viewing history (for recommendations)
- ❌ Passwords (handled by Google)
- ❌ Personal browsing data outside the app

---

## 🤝 Contributing

We welcome contributions from the community! Here's how you can help:

### 🐛 Bug Reports
Found a bug? Please [create an issue](https://github.com/Najaf-Ali-Imran/Cinephile/issues) with:
- Detailed description of the problem
- Steps to reproduce
- Your system info (OS, Qt version)
- Screenshots if applicable

### 💡 Feature Requests
Have an idea? [Open a feature request](https://github.com/Najaf-Ali-Imran/Cinephile/issues) and tell us:
- What feature you'd like to see
- Why it would be useful
- How you envision it working

### 🔧 Code Contributions
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Commit your changes (`git commit -m 'Add amazing feature'`)
5. Push to the branch (`git push origin feature/amazing-feature`)
6. Open a Pull Request

### 📝 Development Guidelines
- Follow Qt coding conventions
- Write clear commit messages
- Add comments for complex logic
- Test your changes thoroughly
- Update documentation as needed

---

## 🗓️ Roadmap

### 🎯 Current Version (1.0)
- ✅ Core movie discovery and details
- ✅ Personal library management
- ✅ AI-powered chatbot assistant
- ✅ Watch Movies
- ✅ User authentication and profiles

### 🚀 Upcoming & In Progress (1.1)
- 🔄 **TV Show Season Tracking** - Track episodes and seasons
- 🎨 **Theme Customization** - Multiple UI themes

### 🌟 Future Vision (1.2+)
- 📊 **Advanced Statistics** - Viewing analytics and insights
- 🎮 **Gamification** - Achievement system for movie watching
- 📱 **Mobile Companion App** - Sync with mobile devices
- 🌐 **Multi-language Support** - Localization for global users
- 🎪 **Community Features** - Reviews and discussions

---

## 📊 Project Stats

<div align="center">

![GitHub repo size](https://img.shields.io/github/repo-size/Najaf-Ali-Imran/Cinephile?style=for-the-badge)
![GitHub code size](https://img.shields.io/github/languages/code-size/Najaf-Ali-Imran/Cinephile?style=for-the-badge)
![GitHub stars](https://img.shields.io/github/stars/Najaf-Ali-Imran/Cinephile?style=for-the-badge)
![GitHub forks](https://img.shields.io/github/forks/Najaf-Ali-Imran/Cinephile?style=for-the-badge)
![GitHub issues](https://img.shields.io/github/issues/Najaf-Ali-Imran/Cinephile?style=for-the-badge)

</div>

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2025 Cinephile 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---



## 📞 Support

Need help or have questions?

- 📧 **Email:** aqibhussain2007@gmail.com
---

<div align="center">

**⭐ If you found Cinephile helpful, please give us a star!**

Made with ❤️ by movie lovers, for movie lovers

[🎬 Start Your Cinema Journey Today](https://github.com/Najaf-Ali-Imran/Cinephile/releases)

</div>
