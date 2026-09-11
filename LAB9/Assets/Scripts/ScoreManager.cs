using UnityEngine;
using UnityEngine.UI;

// Tracks the collectible count and reflects it on the on-screen Text.
public class ScoreManager : MonoBehaviour
{
    public static ScoreManager Instance { get; private set; }
    public Text scoreText;

    private int score;

    void Awake()
    {
        Instance = this;
        UpdateText();
    }

    public void AddPoint()
    {
        score++;
        UpdateText();
    }

    private void UpdateText()
    {
        if (scoreText != null)
        {
            scoreText.text = "Score: " + score;
        }
    }
}
