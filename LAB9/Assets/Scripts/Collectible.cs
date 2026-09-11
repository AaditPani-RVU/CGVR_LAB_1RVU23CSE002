using UnityEngine;

// A spinning cube that disappears and scores a point when the player touches it.
public class Collectible : MonoBehaviour
{
    public float spinSpeed = 90f;

    void Update()
    {
        transform.Rotate(Vector3.up, spinSpeed * Time.deltaTime, Space.World);
    }

    void OnTriggerEnter(Collider other)
    {
        if (other.CompareTag("Player"))
        {
            if (ScoreManager.Instance != null)
            {
                ScoreManager.Instance.AddPoint();
            }
            Destroy(gameObject);
        }
    }
}
